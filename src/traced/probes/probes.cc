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

#include <getopt.h>

#include "perfetto/base/logging.h"
#include "perfetto/base/unix_task_runner.h"
#include "perfetto/traced/traced.h"

#include "src/traced/probes/probes_producer.h"

namespace perfetto {

int __attribute__((visibility("default"))) ProbesMain(int argc, char** argv) {
  static struct option long_options[] = {
      {"cleanup-after-crash", no_argument, 0, 'd'}, {nullptr, 0, 0, 0}};
  int option_index;
  int c;
  while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
      case 'd':
        HardResetFtraceState();
        return 0;
      default:
        PERFETTO_ELOG("Usage: %s [--cleanup-after-crash]", argv[0]);
        return 1;
    }
  }

  PERFETTO_LOG("Starting %s service", argv[0]);
  base::UnixTaskRunner task_runner;
  ProbesProducer producer;
  producer.ConnectWithRetries(PERFETTO_PRODUCER_SOCK_NAME, &task_runner);
  task_runner.Run();
  return 0;
}

}  // namespace perfetto
