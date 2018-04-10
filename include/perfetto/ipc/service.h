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

#ifndef INCLUDE_PERFETTO_IPC_SERVICE_H_
#define INCLUDE_PERFETTO_IPC_SERVICE_H_

#include "perfetto/base/logging.h"
#include "perfetto/base/scoped_file.h"
#include "perfetto/ipc/client_info.h"

namespace perfetto {
namespace ipc {

class ServiceDescriptor;

// The base class for all the autogenerated host-side service interfaces.
class Service {
 public:
  virtual ~Service();

  // Overridden by the auto-generated class. Provides the list of methods and
  // the protobuf (de)serialization functions for their arguments.
  virtual const ServiceDescriptor& GetDescriptor() = 0;

  // Invoked when a remote client disconnects. Use client_info() to obtain
  // details about the client that disconnected.
  virtual void OnClientDisconnected() {}

  // Returns the ClientInfo for the current IPC request. Returns an invalid
  // ClientInfo if called outside the scope of an IPC method.
  const ClientInfo& client_info() {
    PERFETTO_DCHECK(client_info_.is_valid());
    return client_info_;
  }

  base::ScopedFile TakeReceivedFD() {
    if (received_fd_)
      return std::move(*received_fd_);
    return base::ScopedFile();
  }

 private:
  friend class HostImpl;
  ClientInfo client_info_;
  // This is a pointer because the received fd needs to remain owned by the
  // ClientConnection, as we will provide it to all method invocations
  // for that client until one of them calls Service::TakeReceivedFD.
  //
  // Different clients might have sent different FDs so this cannot be owned
  // here.
  //
  // Note that this means that there can always only be one outstanding
  // invocation per client that supplies an FD and the client needs to
  // wait for this one to return before calling another one.
  base::ScopedFile* received_fd_;
};

}  // namespace ipc
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_IPC_SERVICE_H_
