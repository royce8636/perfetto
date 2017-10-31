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

#include "base/test/test_task_runner.h"

#include <stdio.h>
#include <unistd.h>

#include "base/logging.h"

// TODO: the current implementation quite hacky as it keeps waking up every 1ms.

namespace perfetto {
namespace base {

TestTaskRunner::TestTaskRunner() = default;

TestTaskRunner::~TestTaskRunner() = default;

void TestTaskRunner::Run() {
  while (RunUntilIdle()) {
  }
}

bool TestTaskRunner::RunUntilIdle() {
  while (!task_queue_.empty()) {
    std::function<void()> closure = std::move(task_queue_.front());
    task_queue_.pop_front();
    closure();
  }

  int res = RunFileDescriptorWatches(100);
  if (res < 0)
    return false;
  return true;
}

bool TestTaskRunner::RunFileDescriptorWatches(int timeout_ms) {
  struct timeval timeout;
  timeout.tv_usec = (timeout_ms % 1000) * 1000L;
  timeout.tv_sec = static_cast<time_t>(timeout_ms / 1000);
  int max_fd = 0;
  fd_set fds = {};
  for (const auto& it : watched_fds_) {
    FD_SET(it.first, &fds);
    max_fd = std::max(max_fd, it.first);
  }
  int res = select(max_fd + 1, &fds, nullptr, nullptr, &timeout);

  if (res < 0) {
    perror("select() failed");
    return false;
  }
  if (res == 0)
    return true;  // timeout
  for (int fd = 0; fd <= max_fd; ++fd) {
    if (!FD_ISSET(fd, &fds))
      continue;
    auto fd_and_callback = watched_fds_.find(fd);
    PERFETTO_DCHECK(fd_and_callback != watched_fds_.end());
    fd_and_callback->second();
  }
  return true;
}

// TaskRunner implementation.
void TestTaskRunner::PostTask(std::function<void()> closure) {
  task_queue_.emplace_back(std::move(closure));
}

void TestTaskRunner::AddFileDescriptorWatch(int fd,
                                            std::function<void()> callback) {
  PERFETTO_DCHECK(fd >= 0);
  PERFETTO_DCHECK(watched_fds_.count(fd) == 0);
  watched_fds_.emplace(fd, std::move(callback));
}

void TestTaskRunner::RemoveFileDescriptorWatch(int fd) {
  PERFETTO_DCHECK(fd >= 0);
  PERFETTO_DCHECK(watched_fds_.count(fd) == 1);
  watched_fds_.erase(fd);
}

}  // namespace base
}  // namespace perfetto
