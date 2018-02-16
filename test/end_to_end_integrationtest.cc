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

#include <gtest/gtest.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>

#include "perfetto/base/logging.h"
#include "perfetto/trace/trace_packet.pb.h"
#include "perfetto/trace/trace_packet.pbzero.h"
#include "perfetto/traced/traced.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/trace_config.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "perfetto/tracing/ipc/consumer_ipc_client.h"
#include "perfetto/tracing/ipc/service_ipc_host.h"

#include "src/base/test/test_task_runner.h"
#include "src/traced/probes/ftrace_producer.h"
#include "test/fake_consumer.h"
#include "test/fake_producer.h"
#include "test/task_runner_thread.h"

#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID)
#include "perfetto/base/android_task_runner.h"
#endif

namespace perfetto {

#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID)
using PlatformTaskRunner = base::AndroidTaskRunner;
#else
using PlatformTaskRunner = base::UnixTaskRunner;
#endif

// If we're building on Android and starting the daemons ourselves,
// create the sockets in a world-writable location.
#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID) && \
    PERFETTO_BUILDFLAG(PERFETTO_START_DAEMONS)
#define TEST_PRODUCER_SOCK_NAME "/data/local/tmp/traced_producer"
#define TEST_CONSUMER_SOCK_NAME "/data/local/tmp/traced_consumer"
#else
#define TEST_PRODUCER_SOCK_NAME PERFETTO_PRODUCER_SOCK_NAME
#define TEST_CONSUMER_SOCK_NAME PERFETTO_CONSUMER_SOCK_NAME
#endif

class PerfettoTest : public ::testing::Test {
 public:
  PerfettoTest() {}
  ~PerfettoTest() override = default;

 protected:
  // This is used only in daemon starting integrations tests.
  class ServiceDelegate : public ThreadDelegate {
   public:
    ServiceDelegate() = default;
    ~ServiceDelegate() override = default;

    void Initialize(base::TaskRunner* task_runner) override {
      svc_ = ServiceIPCHost::CreateInstance(task_runner);
      unlink(TEST_PRODUCER_SOCK_NAME);
      unlink(TEST_CONSUMER_SOCK_NAME);
      svc_->Start(TEST_PRODUCER_SOCK_NAME, TEST_CONSUMER_SOCK_NAME);
    }

   private:
    std::unique_ptr<ServiceIPCHost> svc_;
  };

  // This is used only in daemon starting integrations tests.
  class FtraceProducerDelegate : public ThreadDelegate {
   public:
    FtraceProducerDelegate() = default;
    ~FtraceProducerDelegate() override = default;

    void Initialize(base::TaskRunner* task_runner) override {
      producer_.reset(new FtraceProducer);
      producer_->ConnectWithRetries(TEST_PRODUCER_SOCK_NAME, task_runner);
    }

   private:
    std::unique_ptr<FtraceProducer> producer_;
  };

  class FakeProducerDelegate : public ThreadDelegate {
   public:
    FakeProducerDelegate(std::function<void()> connect_callback)
        : connect_callback_(std::move(connect_callback)) {}
    ~FakeProducerDelegate() override = default;

    void Initialize(base::TaskRunner* task_runner) override {
      producer_.reset(new FakeProducer("android.perfetto.FakeProducer"));
      producer_->Connect(TEST_PRODUCER_SOCK_NAME, task_runner,
                         std::move(connect_callback_));
    }

   private:
    std::unique_ptr<FakeProducer> producer_;
    std::function<void()> connect_callback_;
  };
};

// TODO(b/73453011): reenable this on more platforms (including standalone
// Android).
#if defined(PERFETTO_BUILD_WITH_ANDROID)
#define MAYBE_TestFtraceProducer TestFtraceProducer
#else
#define MAYBE_TestFtraceProducer DISABLED_TestFtraceProducer
#endif
TEST_F(PerfettoTest, MAYBE_TestFtraceProducer) {
  base::TestTaskRunner task_runner;
  auto finish = task_runner.CreateCheckpoint("no.more.packets");

  // Setip the TraceConfig for the consumer.
  TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(4096 * 10);
  trace_config.set_duration_ms(10000);

  // Create the buffer for ftrace.
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name("com.google.perfetto.ftrace");
  ds_config->set_target_buffer(0);

  // Setup the config for ftrace.
  auto* ftrace_config = ds_config->mutable_ftrace_config();
  *ftrace_config->add_event_names() = "sched_switch";
  *ftrace_config->add_event_names() = "bar";

  // Create the function to handle packets as they come in.
  uint64_t total = 0;
  auto function = [&total, &finish](std::vector<TracePacket> packets,
                                    bool has_more) {
    if (has_more) {
      for (auto& packet : packets) {
        packet.Decode();
        ASSERT_TRUE(packet->has_ftrace_events());
        for (int ev = 0; ev < packet->ftrace_events().event_size(); ev++) {
          ASSERT_TRUE(packet->ftrace_events().event(ev).has_sched_switch());
        }
      }
      total += packets.size();

      // TODO(lalitm): renable this when stiching inside the service is present.
      // ASSERT_FALSE(packets->empty());
    } else {
      ASSERT_GE(total, static_cast<uint64_t>(sysconf(_SC_NPROCESSORS_CONF)));
      ASSERT_TRUE(packets.empty());
      finish();
    }
  };

#if PERFETTO_BUILDFLAG(PERFETTO_START_DAEMONS)
  TaskRunnerThread service_thread;
  service_thread.Start(std::unique_ptr<ServiceDelegate>(new ServiceDelegate));

  TaskRunnerThread producer_thread;
  producer_thread.Start(
      std::unique_ptr<FtraceProducerDelegate>(new FtraceProducerDelegate));
#endif

  // Finally, make the consumer connect to the service.
  FakeConsumer consumer(trace_config, std::move(function), &task_runner);
  consumer.Connect(TEST_CONSUMER_SOCK_NAME);

  // TODO(skyostil): There's a race here before the service processes our data
  // and the consumer tries to retrieve it. For now wait a bit until the service
  // is done, but we should add explicit flushing to avoid this.
  task_runner.PostDelayedTask([&consumer]() { consumer.ReadTraceData(); },
                              13000);

  task_runner.RunUntilCheckpoint("no.more.packets", 20000);
}

// TODO(b/73453011): reenable this on more platforms (including standalone
// Android).
#if defined(PERFETTO_BUILD_WITH_ANDROID)
#define MAYBE_TestFakeProducer TestFakeProducer
#else
#define MAYBE_TestFakeProducer DISABLED_TestFakeProducer
#endif
TEST_F(PerfettoTest, MAYBE_TestFakeProducer) {
  base::TestTaskRunner task_runner;
  auto finish = task_runner.CreateCheckpoint("no.more.packets");

  // Setup the TraceConfig for the consumer.
  TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(4096 * 10);
  trace_config.set_duration_ms(200);

  // Create the buffer for ftrace.
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name("android.perfetto.FakeProducer");
  ds_config->set_target_buffer(0);

  // Create the function to handle packets as they come in.
  uint64_t total = 0;
  auto function = [&total, &finish](std::vector<TracePacket> packets,
                                    bool has_more) {
    if (has_more) {
      for (auto& packet : packets) {
        packet.Decode();
        ASSERT_TRUE(packet->has_for_testing());
        ASSERT_EQ(protos::TracePacket::kTrustedUid,
                  packet->optional_trusted_uid_case());
        ASSERT_EQ(packet->for_testing().str(), "test");
      }
      total += packets.size();

      // TODO(lalitm): renable this when stiching inside the service is present.
      // ASSERT_FALSE(packets->empty());
    } else {
      ASSERT_EQ(total, 10u);
      ASSERT_TRUE(packets.empty());
      finish();
    }
  };

#if PERFETTO_BUILDFLAG(PERFETTO_START_DAEMONS)
  TaskRunnerThread service_thread;
  service_thread.Start(std::unique_ptr<ServiceDelegate>(new ServiceDelegate));
#endif

  auto data_produced = task_runner.CreateCheckpoint("data.produced");
  TaskRunnerThread producer_thread;
  producer_thread.Start(std::unique_ptr<FakeProducerDelegate>(
      new FakeProducerDelegate([&task_runner, &data_produced] {
        task_runner.PostTask(data_produced);
      })));

  // Finally, make the consumer connect to the service.
  FakeConsumer consumer(trace_config, std::move(function), &task_runner);
  consumer.Connect(TEST_CONSUMER_SOCK_NAME);

  task_runner.RunUntilCheckpoint("data.produced");
  consumer.ReadTraceData();

  task_runner.RunUntilCheckpoint("no.more.packets");
}

}  // namespace perfetto
