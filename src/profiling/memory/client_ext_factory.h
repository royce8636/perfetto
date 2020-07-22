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

#ifndef SRC_PROFILING_MEMORY_CLIENT_EXT_FACTORY_H_
#define SRC_PROFILING_MEMORY_CLIENT_EXT_FACTORY_H_

#include <memory>

#include "src/profiling/memory/client.h"
#include "src/profiling/memory/unhooked_allocator.h"

namespace perfetto {
namespace profiling {

void StartHeapprofdIfStatic();

std::shared_ptr<Client> ConstructClient(
    UnhookedAllocator<perfetto::profiling::Client> unhooked_allocator);

}  // namespace profiling
}  // namespace perfetto

#endif  // SRC_PROFILING_MEMORY_CLIENT_EXT_FACTORY_H_
