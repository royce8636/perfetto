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

#include "src/trace_processor/importers/common/process_tracker.h"

#include "perfetto/base/logging.h"
#include "perfetto/ext/base/optional.h"
#include "src/trace_processor/importers/common/args_tracker.h"
#include "src/trace_processor/importers/common/event_tracker.h"
#include "test/gtest_and_gmock.h"

namespace perfetto {
namespace trace_processor {
namespace {

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;

class ProcessTrackerTest : public ::testing::Test {
 public:
  ProcessTrackerTest() {
    context.storage.reset(new TraceStorage());
    context.global_args_tracker.reset(new GlobalArgsTracker(&context));
    context.args_tracker.reset(new ArgsTracker(&context));
    context.process_tracker.reset(new ProcessTracker(&context));
    context.event_tracker.reset(new EventTracker(&context));
  }

 protected:
  TraceProcessorContext context;
};

TEST_F(ProcessTrackerTest, PushProcess) {
  context.process_tracker->SetProcessMetadata(1, base::nullopt, "test",
                                              base::StringView());
  auto pair_it = context.process_tracker->UpidsForPidForTesting(1);
  ASSERT_EQ(pair_it.first->second, 1u);
}

TEST_F(ProcessTrackerTest, GetOrCreateNewProcess) {
  auto upid = context.process_tracker->GetOrCreateProcess(123);
  ASSERT_EQ(context.process_tracker->GetOrCreateProcess(123), upid);
}

TEST_F(ProcessTrackerTest, StartNewProcess) {
  auto upid =
      context.process_tracker->StartNewProcess(1000, 0u, 123, kNullStringId);
  ASSERT_EQ(context.process_tracker->GetOrCreateProcess(123), upid);
  ASSERT_EQ(context.storage->process_table().start_ts()[upid], 1000);
}

TEST_F(ProcessTrackerTest, PushTwoProcessEntries_SamePidAndName) {
  context.process_tracker->SetProcessMetadata(1, base::nullopt, "test",
                                              base::StringView());
  context.process_tracker->SetProcessMetadata(1, base::nullopt, "test",
                                              base::StringView());
  auto pair_it = context.process_tracker->UpidsForPidForTesting(1);
  ASSERT_EQ(pair_it.first->second, 1u);
  ASSERT_EQ(++pair_it.first, pair_it.second);
}

TEST_F(ProcessTrackerTest, PushTwoProcessEntries_DifferentPid) {
  context.process_tracker->SetProcessMetadata(1, base::nullopt, "test",
                                              base::StringView());
  context.process_tracker->SetProcessMetadata(3, base::nullopt, "test",
                                              base::StringView());
  auto pair_it = context.process_tracker->UpidsForPidForTesting(1);
  ASSERT_EQ(pair_it.first->second, 1u);
  auto second_pair_it = context.process_tracker->UpidsForPidForTesting(3);
  ASSERT_EQ(second_pair_it.first->second, 2u);
}

TEST_F(ProcessTrackerTest, AddProcessEntry_CorrectName) {
  context.process_tracker->SetProcessMetadata(1, base::nullopt, "test",
                                              base::StringView());
  auto name =
      context.storage->GetString(context.storage->process_table().name()[1]);
  ASSERT_EQ(name, "test");
}

TEST_F(ProcessTrackerTest, UpdateThreadCreate) {
  context.process_tracker->UpdateThread(12, 2);

  // We expect 3 threads: Invalid thread, main thread for pid, tid 12.
  ASSERT_EQ(context.storage->thread_table().row_count(), 3u);

  auto tid_it = context.process_tracker->UtidsForTidForTesting(12);
  ASSERT_NE(tid_it.first, tid_it.second);
  ASSERT_EQ(context.storage->thread_table().upid()[1].value(), 1u);
  auto pid_it = context.process_tracker->UpidsForPidForTesting(2);
  ASSERT_NE(pid_it.first, pid_it.second);
  ASSERT_EQ(context.storage->process_table().row_count(), 2u);
}

TEST_F(ProcessTrackerTest, PidReuseWithoutStartAndEndThread) {
  UniquePid p1 = context.process_tracker->StartNewProcess(
      base::nullopt, base::nullopt, /*pid=*/1, kNullStringId);
  UniqueTid t1 = context.process_tracker->UpdateThread(/*tid=*/2, /*pid=*/1);

  UniquePid p2 = context.process_tracker->StartNewProcess(
      base::nullopt, base::nullopt, /*pid=*/1, kNullStringId);
  UniqueTid t2 = context.process_tracker->UpdateThread(/*tid=*/2, /*pid=*/1);

  ASSERT_NE(p1, p2);
  ASSERT_NE(t1, t2);

  // We expect 3 processes: idle process, 2x pid 1.
  ASSERT_EQ(context.storage->process_table().row_count(), 3u);
  // We expect 5 threads: Invalid thread, 2x (main thread + sub thread).
  ASSERT_EQ(context.storage->thread_table().row_count(), 5u);
}

TEST_F(ProcessTrackerTest, Cmdline) {
  UniquePid upid = context.process_tracker->SetProcessMetadata(
      1, base::nullopt, "test", "cmdline blah");
  ASSERT_EQ(context.storage->process_table().cmdline().GetString(upid),
            "cmdline blah");
}

TEST_F(ProcessTrackerTest, UpdateThreadName) {
  auto name1 = context.storage->InternString("name1");
  auto name2 = context.storage->InternString("name2");
  auto name3 = context.storage->InternString("name3");

  context.process_tracker->UpdateThreadName(1, name1,
                                            ThreadNamePriority::kFtrace);
  ASSERT_EQ(context.storage->thread_table().row_count(), 2u);
  ASSERT_EQ(context.storage->thread_table().name()[1], name1);

  context.process_tracker->UpdateThreadName(1, name2,
                                            ThreadNamePriority::kProcessTree);
  // The priority is higher: the name should change.
  ASSERT_EQ(context.storage->thread_table().row_count(), 2u);
  ASSERT_EQ(context.storage->thread_table().name()[1], name2);

  context.process_tracker->UpdateThreadName(1, name3,
                                            ThreadNamePriority::kFtrace);
  // The priority is lower: the name should stay the same.
  ASSERT_EQ(context.storage->thread_table().row_count(), 2u);
  ASSERT_EQ(context.storage->thread_table().name()[1], name2);
}

TEST_F(ProcessTrackerTest, SetStartTsIfUnset) {
  auto upid = context.process_tracker->StartNewProcess(
      /*timestamp=*/base::nullopt, 0u, 123, kNullStringId);
  context.process_tracker->SetStartTsIfUnset(upid, 1000);
  ASSERT_EQ(context.storage->process_table().start_ts()[upid], 1000);

  context.process_tracker->SetStartTsIfUnset(upid, 3000);
  ASSERT_EQ(context.storage->process_table().start_ts()[upid], 1000);
}

TEST_F(ProcessTrackerTest, PidReuseAfterExplicitEnd) {
  UniquePid upid = context.process_tracker->GetOrCreateProcess(123);
  context.process_tracker->EndThread(100, 123);

  UniquePid reuse = context.process_tracker->GetOrCreateProcess(123);
  ASSERT_NE(upid, reuse);
}

TEST_F(ProcessTrackerTest, TidReuseAfterExplicitEnd) {
  UniqueTid utid = context.process_tracker->UpdateThread(123, 123);
  context.process_tracker->EndThread(100, 123);

  UniqueTid reuse = context.process_tracker->UpdateThread(123, 123);
  ASSERT_NE(utid, reuse);

  UniqueTid reuse_again = context.process_tracker->UpdateThread(123, 123);
  ASSERT_EQ(reuse, reuse_again);
}

}  // namespace
}  // namespace trace_processor
}  // namespace perfetto
