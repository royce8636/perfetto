/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "src/tracing/test/test_shared_memory.h"

#include <stdlib.h>
#include <string.h>

#include "perfetto/base/logging.h"

namespace perfetto {

TestSharedMemory::TestSharedMemory(size_t size) {
  void* ptr = nullptr;
  int res = posix_memalign(&ptr, 4096, size);
  PERFETTO_CHECK(res == 0 && ptr);
  mem_.reset(ptr);
  memset(ptr, 0, size);
  size_ = size;
}

TestSharedMemory::~TestSharedMemory() {}

TestSharedMemory::Factory::~Factory() {}

std::unique_ptr<SharedMemory> TestSharedMemory::Factory::CreateSharedMemory(
    size_t size) {
  return std::unique_ptr<SharedMemory>(new TestSharedMemory(size));
}

}  // namespace perfetto
