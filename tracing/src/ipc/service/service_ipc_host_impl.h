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

#ifndef TRACING_SRC_IPC_IPC_SERVICE_HOST_IMPL_H_
#define TRACING_SRC_IPC_IPC_SERVICE_HOST_IMPL_H_

#include <memory>

#include "tracing/ipc/service_ipc_host.h"

namespace perfetto {

namespace ipc {
class Host;
}

// The implementation of the IPC host for the tracing service. This class does
// very few things: it mostly initializes the IPC transport. The actual
// implementation of the IPC <> Service business logic glue lives in
// producer_ipc_service.cc and consumer_ipc_service.cc.
class ServiceIPCHostImpl : public ServiceIPCHost {
 public:
  ServiceIPCHostImpl(base::TaskRunner*);
  ~ServiceIPCHostImpl() override;

  // ServiceIPCHost implementation.
  bool Start(const char* producer_socket_name) override;

  Service* service_for_testing() const;

 private:
  base::TaskRunner* const task_runner_;
  std::unique_ptr<Service> svc_;  // The service business logic.

  // The IPC host that listens on the Producer socket. It owns the
  // PosixServiceProducerPort instance which deals with all producers' IPC(s).
  std::unique_ptr<ipc::Host> producer_ipc_port_;
};

}  // namespace perfetto

#endif  // TRACING_SRC_IPC_IPC_SERVICE_HOST_IMPL_H_
