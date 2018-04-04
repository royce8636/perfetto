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

#include "src/tracing/core/service_impl.h"

#include <string.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "perfetto/base/file_utils.h"
#include "perfetto/base/temp_file.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/data_source_config.h"
#include "perfetto/tracing/core/data_source_descriptor.h"
#include "perfetto/tracing/core/producer.h"
#include "perfetto/tracing/core/shared_memory.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "perfetto/tracing/core/trace_writer.h"
#include "src/base/test/test_task_runner.h"
#include "src/tracing/test/mock_consumer.h"
#include "src/tracing/test/mock_producer.h"
#include "src/tracing/test/test_shared_memory.h"

#include "perfetto/trace/test_event.pbzero.h"
#include "perfetto/trace/trace.pb.h"
#include "perfetto/trace/trace_packet.pb.h"
#include "perfetto/trace/trace_packet.pbzero.h"

using ::testing::_;
using ::testing::Contains;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Mock;
using ::testing::Property;
using ::testing::StrictMock;

namespace perfetto {

class ServiceImplTest : public testing::Test {
 public:
  ServiceImplTest() {
    auto shm_factory =
        std::unique_ptr<SharedMemory::Factory>(new TestSharedMemory::Factory());
    svc.reset(static_cast<ServiceImpl*>(
        Service::CreateInstance(std::move(shm_factory), &task_runner)
            .release()));
  }

  std::unique_ptr<MockProducer> CreateMockProducer() {
    return std::unique_ptr<MockProducer>(
        new StrictMock<MockProducer>(&task_runner));
  }

  std::unique_ptr<MockConsumer> CreateMockConsumer() {
    return std::unique_ptr<MockConsumer>(
        new StrictMock<MockConsumer>(&task_runner));
  }

  base::TestTaskRunner task_runner;
  std::unique_ptr<ServiceImpl> svc;
};

TEST_F(ServiceImplTest, RegisterAndUnregister) {
  std::unique_ptr<MockProducer> mock_producer_1 = CreateMockProducer();
  std::unique_ptr<MockProducer> mock_producer_2 = CreateMockProducer();

  mock_producer_1->Connect(svc.get(), "mock_producer_1", 123u /* uid */);
  mock_producer_2->Connect(svc.get(), "mock_producer_2", 456u /* uid */);

  ASSERT_EQ(2u, svc->num_producers());
  ASSERT_EQ(mock_producer_1->endpoint(), svc->GetProducer(1));
  ASSERT_EQ(mock_producer_2->endpoint(), svc->GetProducer(2));
  ASSERT_EQ(123u, svc->GetProducer(1)->uid_);
  ASSERT_EQ(456u, svc->GetProducer(2)->uid_);

  mock_producer_1->RegisterDataSource("foo");
  mock_producer_2->RegisterDataSource("bar");

  mock_producer_1->UnregisterDataSource("foo");
  mock_producer_2->UnregisterDataSource("bar");

  mock_producer_1.reset();
  ASSERT_EQ(1u, svc->num_producers());
  ASSERT_EQ(nullptr, svc->GetProducer(1));

  mock_producer_2.reset();
  ASSERT_EQ(nullptr, svc->GetProducer(2));

  ASSERT_EQ(0u, svc->num_producers());
}

TEST_F(ServiceImplTest, EnableAndDisableTracing) {
  std::unique_ptr<MockConsumer> consumer = CreateMockConsumer();
  consumer->Connect(svc.get());

  std::unique_ptr<MockProducer> producer = CreateMockProducer();
  producer->Connect(svc.get(), "mock_producer");
  producer->RegisterDataSource("data_source");

  TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(128);
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name("data_source");
  consumer->EnableTracing(trace_config);

  producer->WaitForTracingSetup();
  producer->WaitForDataSourceStart("data_source");

  consumer->DisableTracing();
  producer->WaitForDataSourceStop("data_source");
  consumer->WaitForTracingDisabled();
}

TEST_F(ServiceImplTest, LockdownMode) {
  std::unique_ptr<MockConsumer> consumer = CreateMockConsumer();
  consumer->Connect(svc.get());

  std::unique_ptr<MockProducer> producer = CreateMockProducer();
  producer->Connect(svc.get(), "mock_producer_sameuid", geteuid());
  producer->RegisterDataSource("data_source");

  TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(128);
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name("data_source");
  trace_config.set_lockdown_mode(
      TraceConfig::LockdownModeOperation::LOCKDOWN_SET);
  consumer->EnableTracing(trace_config);

  producer->WaitForTracingSetup();
  producer->WaitForDataSourceStart("data_source");

  std::unique_ptr<MockProducer> producer_otheruid = CreateMockProducer();
  auto x = svc->ConnectProducer(producer_otheruid.get(), geteuid() + 1,
                                "mock_producer_ouid");
  EXPECT_CALL(*producer_otheruid, OnConnect()).Times(0);
  task_runner.RunUntilIdle();
  Mock::VerifyAndClearExpectations(producer_otheruid.get());

  consumer->DisableTracing();
  consumer->FreeBuffers();
  producer->WaitForDataSourceStop("data_source");
  consumer->WaitForTracingDisabled();

  trace_config.set_lockdown_mode(
      TraceConfig::LockdownModeOperation::LOCKDOWN_CLEAR);
  consumer->EnableTracing(trace_config);
  producer->WaitForDataSourceStart("data_source");

  std::unique_ptr<MockProducer> producer_otheruid2 = CreateMockProducer();
  producer_otheruid->Connect(svc.get(), "mock_producer_ouid2", geteuid() + 1);

  consumer->DisableTracing();
  producer->WaitForDataSourceStop("data_source");
  consumer->WaitForTracingDisabled();
}

TEST_F(ServiceImplTest, DisconnectConsumerWhileTracing) {
  std::unique_ptr<MockConsumer> consumer = CreateMockConsumer();
  consumer->Connect(svc.get());

  std::unique_ptr<MockProducer> producer = CreateMockProducer();
  producer->Connect(svc.get(), "mock_producer");
  producer->RegisterDataSource("data_source");

  TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(128);
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name("data_source");
  consumer->EnableTracing(trace_config);

  producer->WaitForTracingSetup();
  producer->WaitForDataSourceStart("data_source");

  // Disconnecting the consumer while tracing should trigger data source
  // teardown.
  consumer.reset();
  producer->WaitForDataSourceStop("data_source");
}

TEST_F(ServiceImplTest, ReconnectProducerWhileTracing) {
  std::unique_ptr<MockConsumer> consumer = CreateMockConsumer();
  consumer->Connect(svc.get());

  std::unique_ptr<MockProducer> producer = CreateMockProducer();
  producer->Connect(svc.get(), "mock_producer");
  producer->RegisterDataSource("data_source");

  TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(128);
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name("data_source");
  consumer->EnableTracing(trace_config);

  producer->WaitForTracingSetup();
  producer->WaitForDataSourceStart("data_source");

  // Disconnecting and reconnecting a producer with a matching data source.
  // The Producer should see that data source getting enabled again.
  producer.reset();
  producer = CreateMockProducer();
  producer->Connect(svc.get(), "mock_producer_2");
  producer->RegisterDataSource("data_source");
  producer->WaitForTracingSetup();
  producer->WaitForDataSourceStart("data_source");
}

TEST_F(ServiceImplTest, ProducerIDWrapping) {
  std::vector<std::unique_ptr<MockProducer>> producers;
  producers.push_back(nullptr);

  auto connect_producer_and_get_id = [&producers,
                                      this](const std::string& name) {
    producers.emplace_back(CreateMockProducer());
    producers.back()->Connect(svc.get(), "mock_producer_" + name);
    return svc->last_producer_id_;
  };

  // Connect producers 1-4.
  for (ProducerID i = 1; i <= 4; i++)
    ASSERT_EQ(i, connect_producer_and_get_id(std::to_string(i)));

  // Disconnect producers 1,3.
  producers[1].reset();
  producers[3].reset();

  svc->last_producer_id_ = kMaxProducerID - 1;
  ASSERT_EQ(kMaxProducerID, connect_producer_and_get_id("maxid"));
  ASSERT_EQ(1u, connect_producer_and_get_id("1_again"));
  ASSERT_EQ(3u, connect_producer_and_get_id("3_again"));
  ASSERT_EQ(5u, connect_producer_and_get_id("5"));
  ASSERT_EQ(6u, connect_producer_and_get_id("6"));
}

TEST_F(ServiceImplTest, WriteIntoFileAndStopOnMaxSize) {
  std::unique_ptr<MockConsumer> consumer = CreateMockConsumer();
  consumer->Connect(svc.get());

  std::unique_ptr<MockProducer> producer = CreateMockProducer();
  producer->Connect(svc.get(), "mock_producer");
  producer->RegisterDataSource("data_source");

  TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(4096);
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name("data_source");
  ds_config->set_target_buffer(0);
  trace_config.set_write_into_file(true);
  trace_config.set_file_write_period_ms(1);
  const uint64_t kMaxFileSize = 512;
  trace_config.set_max_file_size_bytes(kMaxFileSize);
  base::TempFile tmp_file = base::TempFile::Create();
  consumer->EnableTracing(trace_config, base::ScopedFile(dup(tmp_file.fd())));

  producer->WaitForTracingSetup();
  producer->WaitForDataSourceStart("data_source");

  static const char kPayload[] = "1234567890abcdef-";
  static const int kNumPackets = 10;

  std::unique_ptr<TraceWriter> writer =
      producer->CreateTraceWriter("data_source");
  // All these packets should fit within kMaxFileSize.
  for (int i = 0; i < kNumPackets; i++) {
    auto tp = writer->NewTracePacket();
    std::string payload(kPayload);
    payload.append(std::to_string(i));
    tp->set_for_testing()->set_str(payload.c_str(), payload.size());
  }

  // Finally add a packet that overflows kMaxFileSize. This should cause the
  // implicit stop of the trace and should *not* be written in the trace.
  {
    auto tp = writer->NewTracePacket();
    char big_payload[kMaxFileSize] = "BIG!";
    tp->set_for_testing()->set_str(big_payload, sizeof(big_payload));
  }
  writer->Flush();
  writer.reset();

  consumer->DisableTracing();
  producer->WaitForDataSourceStop("data_source");
  consumer->WaitForTracingDisabled();

  // Verify the contents of the file.
  std::string trace_raw;
  ASSERT_TRUE(base::ReadFile(tmp_file.path().c_str(), &trace_raw));
  protos::Trace trace;
  ASSERT_TRUE(trace.ParseFromString(trace_raw));
  ASSERT_GE(trace.packet_size(), kNumPackets);
  int num_testing_packet = 0;
  for (int i = 0; i < trace.packet_size(); i++) {
    const protos::TracePacket& tp = trace.packet(i);
    if (!tp.has_for_testing())
      continue;
    ASSERT_EQ(kPayload + std::to_string(num_testing_packet++),
              tp.for_testing().str());
  }
}

}  // namespace perfetto
