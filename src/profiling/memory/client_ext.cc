/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "perfetto/profiling/memory/client_ext.h"

#include <inttypes.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <memory>
#include <tuple>
#include <type_traits>

#include "perfetto/base/build_config.h"
#include "perfetto/base/logging.h"
#include "perfetto/ext/base/no_destructor.h"
#include "perfetto/ext/base/unix_socket.h"
#include "perfetto/ext/base/utils.h"

#include "src/profiling/common/proc_utils.h"
#include "src/profiling/memory/client.h"
#include "src/profiling/memory/client_ext_factory.h"
#include "src/profiling/memory/scoped_spinlock.h"
#include "src/profiling/memory/unhooked_allocator.h"
#include "src/profiling/memory/wire_protocol.h"

using perfetto::profiling::ScopedSpinlock;
using perfetto::profiling::UnhookedAllocator;

namespace {
#if defined(__GLIBC__)
const char* getprogname() {
  return program_invocation_short_name;
}
#elif !defined(__BIONIC__)
const char* getprogname() {
  return "";
}
#endif

// Holds the active profiling client. Is empty at the start, or after we've
// started shutting down a profiling session. Hook invocations take shared_ptr
// copies (ensuring that the client stays alive until no longer needed), and do
// nothing if this primary pointer is empty.
//
// This shared_ptr itself is protected by g_client_lock. Note that shared_ptr
// handles are not thread-safe by themselves:
// https://en.cppreference.com/w/cpp/memory/shared_ptr/atomic
//
// To avoid on-destruction re-entrancy issues, this shared_ptr needs to be
// constructed with an allocator that uses the unhooked malloc & free functions.
// See UnhookedAllocator.
//
// We initialize this storage the first time GetClientLocked is called. We
// cannot use a static initializer because that leads to ordering problems
// of the ELF's constructors.

alignas(std::shared_ptr<perfetto::profiling::Client>) char g_client_arr[sizeof(
    std::shared_ptr<perfetto::profiling::Client>)];

bool g_client_init;

std::shared_ptr<perfetto::profiling::Client>* GetClientLocked() {
  if (!g_client_init) {
    new (g_client_arr) std::shared_ptr<perfetto::profiling::Client>;
    g_client_init = true;
  }
  return reinterpret_cast<std::shared_ptr<perfetto::profiling::Client>*>(
      &g_client_arr);
}

constexpr auto kMinHeapId = 1;

struct HeapprofdHeapInfoInternal {
  HeapprofdHeapInfo info;
  std::atomic<bool> ready;
  std::atomic<bool> enabled;
  std::atomic<uint32_t> service_heap_id;
};

HeapprofdHeapInfoInternal g_heaps[256];

HeapprofdHeapInfoInternal& GetHeap(uint32_t id) {
  return g_heaps[id];
}

// Protects g_client, and serves as an external lock for sampling decisions (see
// perfetto::profiling::Sampler).
//
// We rely on this atomic's destuction being a nop, as it is possible for the
// hooks to attempt to acquire the spinlock after its destructor should have run
// (technically a use-after-destruct scenario).
std::atomic<bool> g_client_lock{false};

std::atomic<uint32_t> g_next_heap_id{kMinHeapId};


// Called only if |g_client_lock| acquisition fails, which shouldn't happen
// unless we're in a completely unexpected state (which we won't know how to
// recover from). Tries to abort (SIGABRT) the whole process to serve as an
// explicit indication of a bug.
//
// Doesn't use PERFETTO_FATAL as that is a single attempt to self-signal (in
// practice - SIGTRAP), while abort() tries to make sure the process has
// exited one way or another.
__attribute__((noreturn, noinline)) void AbortOnSpinlockTimeout() {
  PERFETTO_ELOG(
      "Timed out on the spinlock - something is horribly wrong. "
      "Aborting whole process.");
  abort();
}

// Note: g_client can be reset by heapprofd_initialize without calling this
// function.

void DisableAllHeaps() {
  for (uint32_t i = kMinHeapId; i < g_next_heap_id.load(); ++i) {
    HeapprofdHeapInfoInternal& heap = GetHeap(i);
    if (!heap.ready.load(std::memory_order_acquire))
      continue;
    if (heap.enabled.load(std::memory_order_acquire)) {
      heap.enabled.store(false, std::memory_order_release);
      if (heap.info.callback)
        heap.info.callback(false);
    }
  }
}

void ShutdownLazy(const std::shared_ptr<perfetto::profiling::Client>& client) {
  ScopedSpinlock s(&g_client_lock, ScopedSpinlock::Mode::Try);
  if (PERFETTO_UNLIKELY(!s.locked()))
    AbortOnSpinlockTimeout();

  // other invocation already initiated shutdown
  if (*GetClientLocked() != client)
    return;

  DisableAllHeaps();
  // Clear primary shared pointer, such that later hook invocations become nops.
  GetClientLocked()->reset();
}

// We're a library loaded into a potentially-multithreaded process, which might
// not be explicitly aware of this possiblity. Deadling with forks/clones is
// extremely complicated in such situations, but we attempt to handle certain
// cases.
//
// There are two classes of forking processes to consider:
//  * well-behaved processes that fork only when their threads (if any) are at a
//    safe point, and therefore not in the middle of our hooks/client.
//  * processes that fork with other threads in an arbitrary state. Though
//    technically buggy, such processes exist in practice.
//
// This atfork handler follows a crude lowest-common-denominator approach, where
// to handle the latter class of processes, we systematically leak any |Client|
// state (present only when actively profiling at the time of fork) in the
// postfork-child path.
//
// The alternative with acquiring all relevant locks in the prefork handler, and
// releasing the state postfork handlers, poses a separate class of edge cases,
// and is not deemed to be better as a result.
//
// Notes:
// * this atfork handler fires only for the |fork| libc entrypoint, *not*
//   |clone|. See client.cc's |IsPostFork| for some best-effort detection
//   mechanisms for clone/vfork.
// * it should be possible to start a new profiling session in this child
//   process, modulo the bionic's heapprofd-loading state machine being in the
//   right state.
// * we cannot avoid leaks in all cases anyway (e.g. during shutdown sequence,
//   when only individual straggler threads hold onto the Client).
void AtForkChild() {
  PERFETTO_LOG("heapprofd_client: handling atfork.");

  // A thread (that has now disappeared across the fork) could have been holding
  // the spinlock. We're now the only thread post-fork, so we can reset the
  // spinlock, though the state it protects (the |g_client| shared_ptr) might
  // not be in a consistent state.
  g_client_lock.store(false);

  DisableAllHeaps();

  // Leak the existing shared_ptr contents, including the profiling |Client| if
  // profiling was active at the time of the fork.
  // Note: this code assumes that the creation of the empty shared_ptr does not
  // allocate, which should be the case for all implementations as the
  // constructor has to be noexcept.
  new (g_client_arr) std::shared_ptr<perfetto::profiling::Client>();
}

}  // namespace

__attribute__((visibility("default"))) uint32_t heapprofd_register_heap(
    const HeapprofdHeapInfo* info,
    size_t n) {
  // For backwards compatibility, we handle HeapprofdHeapInfo that are shorter
  // than the current one (and assume all new fields are unset). If someone
  // calls us with a *newer* HeapprofdHeapInfo than this version of the library
  // understands, error out.
  if (n > sizeof(HeapprofdHeapInfo)) {
    return 0;
  }
  uint32_t next_id = g_next_heap_id.fetch_add(1);
  if (next_id >= perfetto::base::ArraySize(g_heaps)) {
    return 0;
  }

  if (next_id == kMinHeapId)
    perfetto::profiling::StartHeapprofdIfStatic();

  HeapprofdHeapInfoInternal& heap = GetHeap(next_id);
  memcpy(&heap.info, info, n);
  heap.ready.store(true, std::memory_order_release);
  return next_id;
}

__attribute__((visibility("default"))) bool
heapprofd_report_allocation(uint32_t heap_id, uint64_t id, uint64_t size) {
  const HeapprofdHeapInfoInternal& heap = GetHeap(heap_id);
  if (!heap.enabled.load(std::memory_order_acquire)) {
    return false;
  }
  size_t sampled_alloc_sz = 0;
  std::shared_ptr<perfetto::profiling::Client> client;
  {
    ScopedSpinlock s(&g_client_lock, ScopedSpinlock::Mode::Try);
    if (PERFETTO_UNLIKELY(!s.locked()))
      AbortOnSpinlockTimeout();

    auto* g_client_ptr = GetClientLocked();
    if (!*g_client_ptr)  // no active client (most likely shutting down)
      return false;

    sampled_alloc_sz =
        (*g_client_ptr)->GetSampleSizeLocked(static_cast<size_t>(size));
    if (sampled_alloc_sz == 0)  // not sampling
      return false;

    client = *g_client_ptr;  // owning copy
  }                          // unlock

  if (!client->RecordMalloc(
          heap.service_heap_id.load(std::memory_order_relaxed),
          sampled_alloc_sz, size, id)) {
    ShutdownLazy(client);
  }
  return true;
}

__attribute__((visibility("default"))) void heapprofd_report_free(
    uint32_t heap_id,
    uint64_t id) {
  const HeapprofdHeapInfoInternal& heap = GetHeap(heap_id);
  if (!heap.enabled.load(std::memory_order_acquire)) {
    return;
  }
  std::shared_ptr<perfetto::profiling::Client> client;
  {
    ScopedSpinlock s(&g_client_lock, ScopedSpinlock::Mode::Try);
    if (PERFETTO_UNLIKELY(!s.locked()))
      AbortOnSpinlockTimeout();

    client = *GetClientLocked();  // owning copy (or empty)
  }

  if (client) {
    if (!client->RecordFree(
            heap.service_heap_id.load(std::memory_order_relaxed), id))
      ShutdownLazy(client);
  }
}

__attribute__((visibility("default"))) bool heapprofd_init_session(
    void* (*malloc_fn)(size_t),
    void (*free_fn)(void*)) {
  static bool first_init = true;
  // Install an atfork handler to deal with *some* cases of the host forking.
  // The handler will be unpatched automatically if we're dlclosed.
  if (first_init && pthread_atfork(/*prepare=*/nullptr, /*parent=*/nullptr,
                                   &AtForkChild) != 0) {
    PERFETTO_PLOG("%s: pthread_atfork failed, not installing hooks.",
                  getprogname());
    return false;
  }
  first_init = false;

  // TODO(fmayer): Check other destructions of client and make a decision
  // whether we want to ban heap objects in the client or not.
  std::shared_ptr<perfetto::profiling::Client> old_client;
  {
    ScopedSpinlock s(&g_client_lock, ScopedSpinlock::Mode::Try);
    if (PERFETTO_UNLIKELY(!s.locked()))
      AbortOnSpinlockTimeout();

    auto* g_client_ptr = GetClientLocked();
    if (*g_client_ptr && (*g_client_ptr)->IsConnected()) {
      PERFETTO_LOG("%s: Rejecting concurrent profiling initialization.",
                   getprogname());
      return true;  // success as we're in a valid state
    }
    old_client = *g_client_ptr;
    g_client_ptr->reset();
  }

  old_client.reset();

  // The dispatch table never changes, so let the custom allocator retain the
  // function pointers directly.
  UnhookedAllocator<perfetto::profiling::Client> unhooked_allocator(malloc_fn,
                                                                    free_fn);

  // These factory functions use heap objects, so we need to run them without
  // the spinlock held.
  std::shared_ptr<perfetto::profiling::Client> client =
      perfetto::profiling::ConstructClient(unhooked_allocator);

  if (!client) {
    PERFETTO_LOG("%s: heapprofd_client not initialized, not installing hooks.",
                 getprogname());
    return false;
  }
  const perfetto::profiling::ClientConfiguration& cli_config =
      client->client_config();

  for (uint32_t j = kMinHeapId; j < g_next_heap_id.load(); ++j) {
    HeapprofdHeapInfoInternal& heap = GetHeap(j);
    if (!heap.ready.load(std::memory_order_acquire))
      continue;

    bool matched = false;
    for (uint32_t i = 0; i < cli_config.num_heaps; ++i) {
      static_assert(sizeof(g_heaps[0].info.heap_name) == HEAPPROFD_HEAP_NAME_SZ,
                    "correct heap name size");
      static_assert(sizeof(cli_config.heaps[0]) == HEAPPROFD_HEAP_NAME_SZ,
                    "correct heap name size");
      if (strncmp(&cli_config.heaps[i][0], &heap.info.heap_name[0],
                  HEAPPROFD_HEAP_NAME_SZ) == 0) {
        heap.service_heap_id.store(i, std::memory_order_relaxed);
        if (!heap.enabled.load(std::memory_order_acquire) && heap.info.callback)
          heap.info.callback(true);
        heap.enabled.store(true, std::memory_order_release);
        matched = true;
        break;
      }
    }
    if (!matched && heap.enabled.load(std::memory_order_acquire)) {
      heap.enabled.store(false, std::memory_order_release);
      if (heap.info.callback)
        heap.info.callback(false);
    }
  }
  PERFETTO_LOG("%s: heapprofd_client initialized.", getprogname());
  {
    ScopedSpinlock s(&g_client_lock, ScopedSpinlock::Mode::Try);
    if (PERFETTO_UNLIKELY(!s.locked()))
      AbortOnSpinlockTimeout();

    // This cannot have been set in the meantime. There are never two concurrent
    // calls to this function, as Bionic uses atomics to guard against that.
    PERFETTO_DCHECK(*GetClientLocked() == nullptr);
    *GetClientLocked() = std::move(client);
  }
  return true;
}
