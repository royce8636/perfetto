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

#ifndef INCLUDE_PERFETTO_TRACED_TRACED_H_
#define INCLUDE_PERFETTO_TRACED_TRACED_H_

#include "perfetto/base/build_config.h"

namespace perfetto {

// TODO(primiano): The actual paths are TBD after security reviews. For the
// moment using an abstract socket on Linux/Andriod and a linked socket on /tmp
// for Mac.

#if BUILDFLAG(OS_ANDROID)
#define PERFETTO_PRODUCER_SOCK_NAME "/dev/socket/traced_producer"
#define PERFETTO_CONSUMER_SOCK_NAME "/dev/socket/traced_consumer"
#else
#define PERFETTO_PRODUCER_SOCK_NAME "/tmp/perfetto-producer"
#define PERFETTO_CONSUMER_SOCK_NAME "/tmp/perfetto-consumer"
#endif

int ServiceMain(int argc, char** argv);
int ProbesMain(int argc, char** argv);
int PerfettoCmdMain(int argc, char** argv);

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACED_TRACED_H_
