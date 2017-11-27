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

#include <stdio.h>
#include <unistd.h>

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/unix_task_runner.h"
#include "ftrace_reader/ftrace_controller.h"
#include "protozero/scattered_stream_writer.h"
#include "scattered_stream_delegate_for_testing.h"

int main(int argc, const char** argv) {
  perfetto::base::UnixTaskRunner runner;
  auto ftrace = perfetto::FtraceController::Create(&runner);

  perfetto::FtraceConfig config;
  for (int i = 1; i < argc; i++) {
    config.AddEvent(argv[i]);
  }
  std::unique_ptr<perfetto::FtraceSink> sink =
      ftrace->CreateSink(std::move(config), nullptr);

  // Sleep for one second so we get some events
  sleep(10);

  perfetto::ScatteredStreamDelegateForTesting buffer(4096);
  protozero::ScatteredStreamWriter stream_writer(&buffer);
  pbzero::FtraceEventBundle message;
  message.Reset(&stream_writer);

  return 0;
}
