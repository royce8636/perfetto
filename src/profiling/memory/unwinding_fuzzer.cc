/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <stddef.h>
#include <stdint.h>

#include "perfetto/base/utils.h"
#include "perfetto/tracing/core/basic_types.h"
#include "src/base/test/test_task_runner.h"
#include "src/profiling/memory/queue_messages.h"
#include "src/profiling/memory/shared_ring_buffer.h"
#include "src/profiling/memory/unwinding.h"

namespace perfetto {
namespace profiling {
namespace {

class NopDelegate : public UnwindingWorker::Delegate {
  void PostAllocRecord(AllocRecord) override {}
  void PostFreeRecord(FreeRecord) override {}
  void PostSocketDisconnected(DataSourceInstanceID, pid_t) override {}
};

int FuzzUnwinding(const uint8_t* data, size_t size) {
  NopDelegate nop_delegate;
  base::TestTaskRunner task_runner;
  UnwindingWorker worker(&nop_delegate, &task_runner);

  SharedRingBuffer::Buffer buf(const_cast<uint8_t*>(data), size);

  pid_t self_pid = getpid();
  DataSourceInstanceID id = 0;
  UnwindingMetadata metadata(self_pid,
                             base::OpenFile("/proc/self/maps", O_RDONLY),
                             base::OpenFile("/proc/self/mem", O_RDONLY));

  worker.HandleBuffer(&buf, &metadata, id, self_pid);
  return 0;
}

}  // namespace
}  // namespace profiling
}  // namespace perfetto

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return perfetto::profiling::FuzzUnwinding(data, size);
}
