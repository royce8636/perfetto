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

#ifndef IPC_INCLUDE_IPC_SERVICE_DESCRIPTOR_H_
#define IPC_INCLUDE_IPC_SERVICE_DESCRIPTOR_H_

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "ipc/basic_types.h"
#include "ipc/deferred.h"

namespace perfetto {
namespace ipc {

class Service;

// This is a pure data structure which holds factory methods and strings for the
// services and their methods that get generated in the .h/.cc files.
// Each autogenerated class has a GetDescriptor() method that returns one
// instance of these and allows both client and hosts to map service and method
// names to IDs and provide function pointers to the protobuf decoder fuctions.
class ServiceDescriptor {
 public:
  struct Method {
    const char* name;

    // DecoderFunc is pointer to a function that takes a string in input
    // containing protobuf encoded data and returns a decoded protobuf message.
    using DecoderFunc = std::unique_ptr<ProtoMessage> (*)(const std::string&);

    // Function pointer to decode the request argument of the method.
    DecoderFunc request_proto_decoder;

    // Function pointer to decoded the reply argument of the method.
    DecoderFunc reply_proto_decoder;
  };

  const char* service_name = nullptr;

  // Note that methods order is not stable. Client and Host might have different
  // method indexes, depending on their versions. The Client can't just rely
  // on the indexes and has to keep a [string -> remote index] translation map.
  std::vector<Method> methods;
};

}  // namespace ipc
}  // namespace perfetto

#endif  // IPC_INCLUDE_IPC_SERVICE_DESCRIPTOR_H_
