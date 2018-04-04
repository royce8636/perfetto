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

#ifndef SRC_TRACING_TEST_MOCK_CONSUMER_H_
#define SRC_TRACING_TEST_MOCK_CONSUMER_H_

#include <memory>

#include "gmock/gmock.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/service.h"
#include "perfetto/tracing/core/trace_packet.h"

#include "perfetto/trace/trace_packet.pb.h"

namespace perfetto {

namespace base {
class TestTaskRunner;
}

class MockConsumer : public Consumer {
 public:
  explicit MockConsumer(base::TestTaskRunner*);
  ~MockConsumer() override;

  void Connect(Service* svc);
  void EnableTracing(const TraceConfig&, base::ScopedFile = base::ScopedFile());
  void DisableTracing();
  void FreeBuffers();
  void WaitForTracingDisabled();
  std::vector<protos::TracePacket> ReadBuffers();

  Service::ConsumerEndpoint* endpoint() { return service_endpoint_.get(); }

  // Consumer implementation.
  MOCK_METHOD0(OnConnect, void());
  MOCK_METHOD0(OnDisconnect, void());
  MOCK_METHOD0(OnTracingDisabled, void());
  MOCK_METHOD2(OnTraceData,
               void(std::vector<TracePacket>* /*packets*/, bool /*has_more*/));

  // gtest doesn't support move-only types. This wrapper is here jut to pass
  // a pointer to the vector (rather than the vector itself) to the mock method.
  void OnTraceData(std::vector<TracePacket> packets, bool has_more) override {
    OnTraceData(&packets, has_more);
  }

 private:
  base::TestTaskRunner* const task_runner_;
  std::unique_ptr<Service::ConsumerEndpoint> service_endpoint_;
};

}  // namespace perfetto

#endif  // SRC_TRACING_TEST_MOCK_CONSUMER_H_
