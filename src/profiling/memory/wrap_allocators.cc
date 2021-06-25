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

#include <stdlib.h>

#include <cinttypes>

#include "perfetto/ext/base/utils.h"
#include "perfetto/heap_profile.h"
#include "src/profiling/memory/wrap_allocators.h"

namespace perfetto {
namespace profiling {

namespace {
size_t RoundUpToSysPageSize(size_t req_size) {
  const size_t page_size = base::GetSysPageSize();
  return (req_size + page_size - 1) & ~(page_size - 1);
}
}  // namespace

void* wrap_malloc(uint32_t heap_id, void* (*fn)(size_t), size_t size) {
  void* addr = fn(size);
  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(addr),
                                size);
  return addr;
}

void* wrap_calloc(uint32_t heap_id,
                  void* (*fn)(size_t, size_t),
                  size_t nmemb,
                  size_t size) {
  void* addr = fn(nmemb, size);
  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(addr),
                                nmemb * size);
  return addr;
}

void* wrap_memalign(uint32_t heap_id,
                    void* (*fn)(size_t, size_t),
                    size_t alignment,
                    size_t size) {
  void* addr = fn(alignment, size);
  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(addr),
                                size);
  return addr;
}

int wrap_posix_memalign(uint32_t heap_id,
                        int (*fn)(void**, size_t, size_t),
                        void** memptr,
                        size_t alignment,
                        size_t size) {
  int res = fn(memptr, alignment, size);
  if (res != 0)
    return res;

  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(*memptr),
                                size);
  return 0;
}

// Note: we record the free before calling the backing implementation to make
// sure that the address is not reused before we've processed the deallocation
// (which includes assigning a sequence id to it).
void wrap_free(uint32_t heap_id, void (*fn)(void*), void* pointer) {
  // free on a nullptr is valid but has no effect. Short circuit here, for
  // various advantages:
  // * More efficient
  // * Notably printf calls free(nullptr) even when it is used in a way
  //   malloc-free way, as it unconditionally frees the pointer even if
  //   it was never written to.
  //   Short circuiting here makes it less likely to accidentally build
  //   infinite recursion.
  if (pointer == nullptr)
    return;

  AHeapProfile_reportFree(heap_id, reinterpret_cast<uint64_t>(pointer));
  return fn(pointer);
}

// As with the free, we record the deallocation before calling the backing
// implementation to make sure the address is still exclusive while we're
// processing it.
void* wrap_realloc(uint32_t heap_id,
                   void* (*fn)(void*, size_t),
                   void* pointer,
                   size_t size) {
  if (pointer)
    AHeapProfile_reportFree(heap_id, reinterpret_cast<uint64_t>(pointer));
  void* addr = fn(pointer, size);
  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(addr),
                                size);
  return addr;
}

void* wrap_pvalloc(uint32_t heap_id, void* (*fn)(size_t), size_t size) {
  void* addr = fn(size);
  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(addr),
                                RoundUpToSysPageSize(size));
  return addr;
}

void* wrap_valloc(uint32_t heap_id, void* (*fn)(size_t), size_t size) {
  void* addr = fn(size);
  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(addr),
                                size);
  return addr;
}

void* wrap_reallocarray(uint32_t heap_id,
                        void* (*fn)(void*, size_t, size_t),
                        void* pointer,
                        size_t nmemb,
                        size_t size) {
  if (pointer)
    AHeapProfile_reportFree(heap_id, reinterpret_cast<uint64_t>(pointer));
  void* addr = fn(pointer, nmemb, size);
  AHeapProfile_reportAllocation(heap_id, reinterpret_cast<uint64_t>(addr),
                                nmemb * size);
  return addr;
}

}  // namespace profiling
}  // namespace perfetto
