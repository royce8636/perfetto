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

#include "src/base/test/test_task_runner.h"
#include "src/ipc/test/test_socket.h"
#include "src/profiling/memory/client.h"
#include "src/profiling/memory/socket_listener.h"
#include "src/profiling/memory/unwinding.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace perfetto {
namespace profiling {
namespace {

constexpr char kSocketName[] = TEST_SOCK_NAME("heapprofd_integrationtest");

void __attribute__((noinline)) OtherFunction(Client* client) {
  client->RecordMalloc(10, 10, 0xf00);
}

void __attribute__((noinline)) SomeFunction(Client* client) {
  OtherFunction(client);
}

class HeapprofdIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override { DESTROY_TEST_SOCK(kSocketName); }
  void TearDown() override { DESTROY_TEST_SOCK(kSocketName); }
};

// ASAN does not like sendmsg of the stack.
// TODO(fmayer): Try to fix this more properly.
#ifndef ADDRESS_SANITIZER
#define MAYBE_EndToEnd EndToEnd
#define MAYBE_MultiSession MultiSession
#else
#define MAYBE_EndToEnd DISABLED_EndToEnd
#define MAYBE_MultiSession DISABLED_MultiSession
#endif

TEST_F(HeapprofdIntegrationTest, MAYBE_EndToEnd) {
  GlobalCallstackTrie callsites;
  // TODO(fmayer): Actually test the dump.
  BookkeepingThread bookkeeping_thread("");

  base::TestTaskRunner task_runner;
  auto done = task_runner.CreateCheckpoint("done");
  constexpr uint64_t kSamplingInterval = 123;
  SocketListener listener(
      [&done, &bookkeeping_thread](UnwindingRecord r) {
        // TODO(fmayer): Test symbolization and result of unwinding.
        // This check will only work on in-tree builds as out-of-tree
        // libunwindstack is behaving a bit weirdly.
// TODO(fmayer): Fix out of tree integration test.
#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID)
        BookkeepingRecord bookkeeping_record;
        ASSERT_TRUE(HandleUnwindingRecord(&r, &bookkeeping_record));
        bookkeeping_thread.HandleBookkeepingRecord(&bookkeeping_record);
#endif
        base::ignore_result(r);
        base::ignore_result(bookkeeping_thread);
        done();
      },
      &bookkeeping_thread);

  auto session = listener.ExpectPID(getpid(), {kSamplingInterval});
  auto sock = base::UnixSocket::Listen(kSocketName, &listener, &task_runner);
  if (!sock->is_listening()) {
    PERFETTO_ELOG("Socket not listening.");
    PERFETTO_CHECK(false);
  }
  std::thread th([kSamplingInterval] {
    Client client(kSocketName, 1);
    SomeFunction(&client);
    EXPECT_EQ(client.client_config_for_testing().interval, kSamplingInterval);
  });

  task_runner.RunUntilCheckpoint("done");
  th.join();
}

TEST_F(HeapprofdIntegrationTest, MAYBE_MultiSession) {
  GlobalCallstackTrie callsites;
  // TODO(fmayer): Actually test the dump.
  BookkeepingThread bookkeeping_thread("");

  base::TestTaskRunner task_runner;
  auto done = task_runner.CreateCheckpoint("done");
  constexpr uint64_t kSamplingInterval = 123;
  SocketListener listener([&done](UnwindingRecord) { done(); },
                          &bookkeeping_thread);

  auto session = listener.ExpectPID(getpid(), {kSamplingInterval});
  // Allow to get a second session, but it will still use the previous
  // sampling rate.
  auto session2 = listener.ExpectPID(getpid(), {kSamplingInterval + 1});
  session = SocketListener::ProfilingSession();
  auto sock = base::UnixSocket::Listen(kSocketName, &listener, &task_runner);
  if (!sock->is_listening()) {
    PERFETTO_ELOG("Socket not listening.");
    PERFETTO_CHECK(false);
  }
  std::thread th([kSamplingInterval] {
    Client client(kSocketName, 1);
    SomeFunction(&client);
    EXPECT_EQ(client.client_config_for_testing().interval, kSamplingInterval);
  });

  task_runner.RunUntilCheckpoint("done");
  th.join();
}

}  // namespace
}  // namespace profiling
}  // namespace perfetto
