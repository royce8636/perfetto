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

#include "src/traced/probes/ftrace/ftrace_config_muxer.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/traced/probes/ftrace/atrace_wrapper.h"
#include "src/traced/probes/ftrace/ftrace_procfs.h"
#include "src/traced/probes/ftrace/proto_translation_table.h"

using testing::_;
using testing::AnyNumber;
using testing::MatchesRegex;
using testing::Contains;
using testing::ElementsAreArray;
using testing::Eq;
using testing::IsEmpty;
using testing::NiceMock;
using testing::Not;
using testing::Return;
using testing::UnorderedElementsAre;

namespace perfetto {
namespace {

class MockFtraceProcfs : public FtraceProcfs {
 public:
  MockFtraceProcfs() : FtraceProcfs("/root/") {
    ON_CALL(*this, NumberOfCpus()).WillByDefault(Return(1));
    ON_CALL(*this, WriteToFile(_, _)).WillByDefault(Return(true));
    ON_CALL(*this, ClearFile(_)).WillByDefault(Return(true));
    EXPECT_CALL(*this, NumberOfCpus()).Times(AnyNumber());
  }

  MOCK_METHOD2(WriteToFile,
               bool(const std::string& path, const std::string& str));
  MOCK_METHOD1(ReadOneCharFromFile, char(const std::string& path));
  MOCK_METHOD1(ClearFile, bool(const std::string& path));
  MOCK_CONST_METHOD1(ReadFileIntoString, std::string(const std::string& path));
  MOCK_CONST_METHOD0(NumberOfCpus, size_t());
};

struct MockRunAtrace {
  MockRunAtrace() {
    static MockRunAtrace* instance;
    instance = this;
    SetRunAtraceForTesting([](const std::vector<std::string>& args) {
      return instance->RunAtrace(args);
    });
  }

  ~MockRunAtrace() { SetRunAtraceForTesting(nullptr); }

  MOCK_METHOD1(RunAtrace, bool(const std::vector<std::string>&));
};

class MockProtoTranslationTable : public ProtoTranslationTable {
 public:
  MockProtoTranslationTable(NiceMock<MockFtraceProcfs>* ftrace_procfs,
                            const std::vector<Event>& events,
                            std::vector<Field> common_fields,
                            FtracePageHeaderSpec ftrace_page_header_spec)
      : ProtoTranslationTable(ftrace_procfs,
                              events,
                              common_fields,
                              ftrace_page_header_spec) {}
  MOCK_METHOD2(GetOrCreateEvent,
               Event*(const std::string& group, const std::string& event));
};

class FtraceConfigMuxerTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockProtoTranslationTable> GetMockTable() {
    std::vector<Field> common_fields;
    std::vector<Event> events;
    return std::unique_ptr<MockProtoTranslationTable>(
        new MockProtoTranslationTable(
            &table_procfs_, events, std::move(common_fields),
            ProtoTranslationTable::DefaultPageHeaderSpecForTesting()));
  }
  std::unique_ptr<ProtoTranslationTable> CreateFakeTable() {
    std::vector<Field> common_fields;
    std::vector<Event> events;
    {
      Event event;
      event.name = "sched_switch";
      event.group = "sched";
      event.ftrace_event_id = 1;
      events.push_back(event);
    }

    {
      Event event;
      event.name = "sched_wakeup";
      event.group = "sched";
      event.ftrace_event_id = 10;
      events.push_back(event);
    }

    {
      Event event;
      event.name = "sched_new";
      event.group = "sched";
      event.ftrace_event_id = 11;
      events.push_back(event);
    }

    {
      Event event;
      event.name = "cgroup_mkdir";
      event.group = "cgroup";
      event.ftrace_event_id = 12;
      events.push_back(event);
    }

    {
      Event event;
      event.name = "mm_vmscan_direct_reclaim_begin";
      event.group = "vmscan";
      event.ftrace_event_id = 13;
      events.push_back(event);
    }

    {
      Event event;
      event.name = "lowmemory_kill";
      event.group = "lowmemorykiller";
      event.ftrace_event_id = 14;
      events.push_back(event);
    }

    {
      Event event;
      event.name = "print";
      event.group = "ftrace";
      event.ftrace_event_id = 20;
      events.push_back(event);
    }

    return std::unique_ptr<ProtoTranslationTable>(new ProtoTranslationTable(
        &table_procfs_, events, std::move(common_fields),
        ProtoTranslationTable::DefaultPageHeaderSpecForTesting()));
  }

  NiceMock<MockFtraceProcfs> table_procfs_;
  std::unique_ptr<ProtoTranslationTable> table_ = CreateFakeTable();
};

TEST_F(FtraceConfigMuxerTest, ComputeCpuBufferSizeInPages) {
  static constexpr size_t kMaxBufSizeInPages = 16 * 1024u;
  // No buffer size given: good default (128 pages = 512kb).
  EXPECT_EQ(ComputeCpuBufferSizeInPages(0), 128u);
  // Buffer size given way too big: good default.
  EXPECT_EQ(ComputeCpuBufferSizeInPages(10 * 1024 * 1024), kMaxBufSizeInPages);
  // The limit is 64mb per CPU, 512mb is too much.
  EXPECT_EQ(ComputeCpuBufferSizeInPages(512 * 1024), kMaxBufSizeInPages);
  // Your size ends up with less than 1 page per cpu -> 1 page.
  EXPECT_EQ(ComputeCpuBufferSizeInPages(3), 1u);
  // You picked a good size -> your size rounded to nearest page.
  EXPECT_EQ(ComputeCpuBufferSizeInPages(42), 10u);
}

TEST_F(FtraceConfigMuxerTest, AddGenericEvent) {
  auto mock_table = GetMockTable();
  MockFtraceProcfs ftrace;

  FtraceConfig config = CreateFtraceConfig({"power/cpu_frequency"});

  FtraceConfigMuxer model(&ftrace, mock_table.get());

  ON_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .WillByDefault(Return("[local] global boot"));
  EXPECT_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .Times(AnyNumber());

  EXPECT_CALL(ftrace, ReadOneCharFromFile("/root/tracing_on"))
      .Times(2)
      .WillRepeatedly(Return('0'));
  EXPECT_CALL(ftrace, WriteToFile("/root/buffer_size_kb", "512"));
  EXPECT_CALL(ftrace, WriteToFile("/root/trace_clock", "boot"));
  EXPECT_CALL(ftrace, WriteToFile("/root/tracing_on", "1"));
  EXPECT_CALL(ftrace,
              WriteToFile("/root/events/power/cpu_frequency/enable", "1"));

  Event event_to_return;
  event_to_return.name = "cpu_frequency";
  event_to_return.group = "power";
  ON_CALL(*mock_table, GetOrCreateEvent("power", "cpu_frequency"))
      .WillByDefault(Return(&event_to_return));
  EXPECT_CALL(*mock_table, GetOrCreateEvent("power", "cpu_frequency"));

  FtraceConfigId id = model.SetupConfig(config);
  ASSERT_TRUE(model.ActivateConfig(id));
  const FtraceConfig* actual_config = model.GetConfig(id);
  EXPECT_TRUE(actual_config);
  EXPECT_THAT(actual_config->ftrace_events(), Contains("cpu_frequency"));
}

TEST_F(FtraceConfigMuxerTest, TurnFtraceOnOff) {
  MockFtraceProcfs ftrace;

  FtraceConfig config = CreateFtraceConfig({"sched_switch", "foo"});

  FtraceConfigMuxer model(&ftrace, table_.get());

  ON_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .WillByDefault(Return("[local] global boot"));
  EXPECT_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .Times(AnyNumber());

  EXPECT_CALL(ftrace, ReadOneCharFromFile("/root/tracing_on"))
      .Times(2)
      .WillRepeatedly(Return('0'));
  EXPECT_CALL(ftrace, WriteToFile("/root/buffer_size_kb", "512"));
  EXPECT_CALL(ftrace, WriteToFile("/root/trace_clock", "boot"));
  EXPECT_CALL(ftrace, WriteToFile("/root/tracing_on", "1"));
  EXPECT_CALL(ftrace,
              WriteToFile("/root/events/sched/sched_switch/enable", "1"));
  FtraceConfigId id = model.SetupConfig(config);
  ASSERT_TRUE(id);
  ASSERT_TRUE(model.ActivateConfig(id));

  const FtraceConfig* actual_config = model.GetConfig(id);
  EXPECT_TRUE(actual_config);
  EXPECT_THAT(actual_config->ftrace_events(), Contains("sched_switch"));
  EXPECT_THAT(actual_config->ftrace_events(), Not(Contains("foo")));

  EXPECT_CALL(ftrace, WriteToFile("/root/tracing_on", "0"));
  EXPECT_CALL(ftrace, WriteToFile("/root/buffer_size_kb", "0"));
  EXPECT_CALL(ftrace, WriteToFile("/root/events/enable", "0"));
  EXPECT_CALL(ftrace,
              WriteToFile("/root/events/sched/sched_switch/enable", "0"));
  EXPECT_CALL(ftrace, ClearFile("/root/trace"));
  EXPECT_CALL(ftrace, ClearFile(MatchesRegex("/root/per_cpu/cpu[0-9]/trace")));
  ASSERT_TRUE(model.RemoveConfig(id));
}

TEST_F(FtraceConfigMuxerTest, FtraceIsAlreadyOn) {
  MockFtraceProcfs ftrace;

  FtraceConfig config = CreateFtraceConfig({"sched_switch"});

  FtraceConfigMuxer model(&ftrace, table_.get());

  // If someone is using ftrace already don't stomp on what they are doing.
  EXPECT_CALL(ftrace, ReadOneCharFromFile("/root/tracing_on"))
      .WillOnce(Return('1'));
  FtraceConfigId id = model.SetupConfig(config);
  ASSERT_FALSE(id);
}

TEST_F(FtraceConfigMuxerTest, Atrace) {
  NiceMock<MockFtraceProcfs> ftrace;
  MockRunAtrace atrace;

  FtraceConfig config = CreateFtraceConfig({"sched_switch"});
  *config.add_atrace_categories() = "sched";

  FtraceConfigMuxer model(&ftrace, table_.get());

  EXPECT_CALL(ftrace, ReadOneCharFromFile("/root/tracing_on"))
      .WillOnce(Return('0'));
  EXPECT_CALL(atrace,
              RunAtrace(ElementsAreArray(
                  {"atrace", "--async_start", "--only_userspace", "sched"})))
      .WillOnce(Return(true));

  FtraceConfigId id = model.SetupConfig(config);
  ASSERT_TRUE(id);

  const FtraceConfig* actual_config = model.GetConfig(id);
  EXPECT_TRUE(actual_config);
  EXPECT_THAT(actual_config->ftrace_events(), Contains("sched_switch"));
  EXPECT_THAT(actual_config->ftrace_events(), Contains("print"));

  EXPECT_CALL(atrace, RunAtrace(ElementsAreArray(
                          {"atrace", "--async_stop", "--only_userspace"})))
      .WillOnce(Return(true));
  ASSERT_TRUE(model.RemoveConfig(id));
}

TEST_F(FtraceConfigMuxerTest, AtraceTwoApps) {
  NiceMock<MockFtraceProcfs> ftrace;
  MockRunAtrace atrace;

  FtraceConfig config = CreateFtraceConfig({});
  *config.add_atrace_apps() = "com.google.android.gms.persistent";
  *config.add_atrace_apps() = "com.google.android.gms";

  FtraceConfigMuxer model(&ftrace, table_.get());

  EXPECT_CALL(ftrace, ReadOneCharFromFile("/root/tracing_on"))
      .WillOnce(Return('0'));
  EXPECT_CALL(
      atrace,
      RunAtrace(ElementsAreArray(
          {"atrace", "--async_start", "--only_userspace", "-a",
           "com.google.android.gms.persistent,com.google.android.gms"})))
      .WillOnce(Return(true));

  FtraceConfigId id = model.SetupConfig(config);
  ASSERT_TRUE(id);

  const FtraceConfig* actual_config = model.GetConfig(id);
  EXPECT_TRUE(actual_config);
  EXPECT_THAT(actual_config->ftrace_events(), Contains("print"));

  EXPECT_CALL(atrace, RunAtrace(ElementsAreArray(
                          {"atrace", "--async_stop", "--only_userspace"})))
      .WillOnce(Return(true));
  ASSERT_TRUE(model.RemoveConfig(id));
}

TEST_F(FtraceConfigMuxerTest, SetupClockForTesting) {
  MockFtraceProcfs ftrace;
  FtraceConfig config;

  FtraceConfigMuxer model(&ftrace, table_.get());

  EXPECT_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .Times(AnyNumber());

  ON_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .WillByDefault(Return("[local] global boot"));
  EXPECT_CALL(ftrace, WriteToFile("/root/trace_clock", "boot"));
  model.SetupClockForTesting(config);

  ON_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .WillByDefault(Return("[local] global"));
  EXPECT_CALL(ftrace, WriteToFile("/root/trace_clock", "global"));
  model.SetupClockForTesting(config);

  ON_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .WillByDefault(Return(""));
  model.SetupClockForTesting(config);

  ON_CALL(ftrace, ReadFileIntoString("/root/trace_clock"))
      .WillByDefault(Return("local [global]"));
  model.SetupClockForTesting(config);
}

TEST_F(FtraceConfigMuxerTest, GetFtraceEvents) {
  FtraceConfig config = CreateFtraceConfig({"sched_switch"});
  std::set<std::string> events = GetFtraceEvents(config, table_.get());

  EXPECT_THAT(events, Contains("sched_switch"));
  EXPECT_THAT(events, Not(Contains("print")));
}

TEST_F(FtraceConfigMuxerTest, GetFtraceEventsAtrace) {
  FtraceConfig config = CreateFtraceConfig({});
  *config.add_atrace_categories() = "sched";
  std::set<std::string> events = GetFtraceEvents(config, table_.get());

  EXPECT_THAT(events, Contains("sched_switch"));
  EXPECT_THAT(events, Contains("sched_cpu_hotplug"));
  EXPECT_THAT(events, Contains("print"));
}

TEST_F(FtraceConfigMuxerTest, GetFtraceEventsAtraceCategories) {
  FtraceConfig config = CreateFtraceConfig({});
  *config.add_atrace_categories() = "sched";
  *config.add_atrace_categories() = "memreclaim";
  std::set<std::string> events = GetFtraceEvents(config, table_.get());

  EXPECT_THAT(events, Contains("sched_switch"));
  EXPECT_THAT(events, Contains("sched_cpu_hotplug"));
  EXPECT_THAT(events, Contains("cgroup_mkdir"));
  EXPECT_THAT(events, Contains("mm_vmscan_direct_reclaim_begin"));
  EXPECT_THAT(events, Contains("lowmemory_kill"));
  EXPECT_THAT(events, Contains("print"));
}

}  // namespace
}  // namespace perfetto
