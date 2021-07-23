/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef TOOLS_PROTO_MERGER_PROTO_FILE_SERIALIZER_H_
#define TOOLS_PROTO_MERGER_PROTO_FILE_SERIALIZER_H_

#include "tools/proto_merger/proto_file.h"

namespace perfetto {
namespace proto_merger {

// Serializes a ProtoFile struct into a .proto file which is
// capable of being parsed by protoc.
// For example:
// ProtoFile {
//   messages: [
//     Message {
//       name: Baz
//       fields: [
//         Field {
//           label: optional
//           type: Foo
//           name: foo
//           number: 1
//         }
//         Field {
//           label: optional
//           type: Bar
//           name: bar
//           number: 2
//         }
//       ]
//     }
//   ]
// }
//
// will convert to:
//
// message Baz {
//   optional Foo foo = 1;
//   optonal Bar bar = 2;
// }
std::string ProtoFileToDotProto(const ProtoFile&);

}  // namespace proto_merger
}  // namespace perfetto

#endif  // TOOLS_PROTO_MERGER_PROTO_FILE_SERIALIZER_H_
