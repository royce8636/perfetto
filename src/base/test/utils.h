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

#ifndef SRC_BASE_TEST_UTILS_H_
#define SRC_BASE_TEST_UTILS_H_

#include <string>

namespace perfetto {
namespace base {

std::string GetTestDataPath(const std::string& path);

}  // namespace base
}  // namespace perfetto

#endif  // SRC_BASE_TEST_UTILS_H_
