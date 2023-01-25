/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef INCLUDE_PERFETTO_TRACING_INTERNAL_SYSTEM_TRACING_BACKEND_H_
#define INCLUDE_PERFETTO_TRACING_INTERNAL_SYSTEM_TRACING_BACKEND_H_

#include "perfetto/base/export.h"
#include "perfetto/tracing/tracing_backend.h"

namespace perfetto {

namespace base {
class TaskRunner;
}

class Producer;

// A built-in implementation of TracingBackend that connects to the system
// tracing daemon (traced) via a UNIX socket using the perfetto built-in
// proto-based IPC mechanism. Instantiated when the embedder calls
// Tracing::Initialize(kSystemBackend). It allows to get app-traces fused
// together with system traces, useful to correlate on the timeline system
// events (e.g. scheduling slices from the kernel) with in-app events.
namespace internal {

// Full backend (with producer and consumer)
class PERFETTO_EXPORT_COMPONENT SystemTracingBackend : public TracingBackend {
 public:
  static TracingBackend* GetInstance();

  // TracingBackend implementation.
  std::unique_ptr<ProducerEndpoint> ConnectProducer(
      const ConnectProducerArgs&) override;
  std::unique_ptr<ConsumerEndpoint> ConnectConsumer(
      const ConnectConsumerArgs&) override;

 private:
  SystemTracingBackend();
};

// Producer only backend.
class PERFETTO_EXPORT_COMPONENT SystemTracingProducerOnlyBackend
    : public TracingBackend {
 public:
  static TracingBackend* GetInstance();

  // TracingBackend implementation.
  std::unique_ptr<ProducerEndpoint> ConnectProducer(
      const ConnectProducerArgs&) override;
  std::unique_ptr<ConsumerEndpoint> ConnectConsumer(
      const ConnectConsumerArgs&) override;

 private:
  SystemTracingProducerOnlyBackend();
};

}  // namespace internal
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_INTERNAL_SYSTEM_TRACING_BACKEND_H_
