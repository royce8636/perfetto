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

#ifndef INCLUDE_PERFETTO_IPC_SERVICE_PROXY_H_
#define INCLUDE_PERFETTO_IPC_SERVICE_PROXY_H_

#include "perfetto/ipc/basic_types.h"

#include <assert.h>

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "perfetto/base/weak_ptr.h"
#include "perfetto/ipc/deferred.h"

namespace perfetto {
namespace ipc {

class Client;
class ServiceDescriptor;

// The base class for the client-side autogenerated stubs that forward method
// invocations to the host. All the methods of this class are meant to be called
// only by the autogenerated code.
class ServiceProxy {
 public:
  class EventListener {
   public:
    virtual ~EventListener() = default;

    // Called once after Client::BindService() if the ServiceProxy has been
    // successfully bound to the host. It is possible to start sending IPC
    // requests soon after this.
    virtual void OnConnect() {}

    // Called if the connection fails to be established or drops after having
    // been established.
    virtual void OnDisconnect() {}
  };

  // Guarantees that no callback will happen after this object has been
  // destroyed. The caller has to guarantee that the |event_listener| stays
  // alive at least as long as the ServiceProxy instance.
  explicit ServiceProxy(EventListener*);
  virtual ~ServiceProxy();

  void InitializeBinding(base::WeakPtr<Client>,
                         ServiceID,
                         std::map<std::string, MethodID>);

  // Called by the IPC methods in the autogenerated classes.
  void BeginInvoke(const std::string& method_name,
                   const ProtoMessage& request,
                   DeferredBase reply);

  // Called by ClientImpl.
  // |reply_args| == nullptr means request failure.
  void EndInvoke(RequestID,
                 std::unique_ptr<ProtoMessage> reply_arg,
                 bool has_more);

  // Called by ClientImpl.
  void OnConnect(bool success);
  void OnDisconnect();
  bool connected() const { return service_id_ != 0; }

  base::WeakPtr<ServiceProxy> GetWeakPtr() const;

  // Implemented by the autogenerated class.
  virtual const ServiceDescriptor& GetDescriptor() = 0;

 private:
  base::WeakPtr<Client> client_;
  ServiceID service_id_ = 0;
  std::map<std::string, MethodID> remote_method_ids_;
  std::map<RequestID, DeferredBase> pending_callbacks_;
  EventListener* const event_listener_;
  base::WeakPtrFactory<ServiceProxy> weak_ptr_factory_;
};

}  // namespace ipc
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_IPC_SERVICE_PROXY_H_
