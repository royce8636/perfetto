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

#ifndef IPC_INCLUDE_IPC_BASIC_TYPES_H_
#define IPC_INCLUDE_IPC_BASIC_TYPES_H_

#include <stdint.h>

namespace google {
namespace protobuf {
class MessageLite;
}  // namespace protobuf
}  // namespace google

namespace perfetto {
namespace ipc {

using ProtoMessage = ::google::protobuf::MessageLite;
using ServiceID = uint64_t;
using MethodID = uint64_t;
using ClientID = uint64_t;
using RequestID = uint64_t;

}  // namespace ipc
}  // namespace perfetto

#endif  // IPC_INCLUDE_IPC_BASIC_TYPES_H_
