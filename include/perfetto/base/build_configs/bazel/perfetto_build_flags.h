/*
 * Copyright (C) 2019 The Android Open Source Project
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

// Generated by write_buildflag_header.py

// fix_include_guards: off
#ifndef GEN_BUILD_CONFIG_PERFETTO_BUILD_FLAGS_H_
#define GEN_BUILD_CONFIG_PERFETTO_BUILD_FLAGS_H_

// clang-format off
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_ANDROID_BUILD() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_CHROMIUM_BUILD() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_STANDALONE_BUILD() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_START_DAEMONS() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_IPC() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_WATCHDOG() (PERFETTO_BUILDFLAG_DEFINE_PERFETTO_OS_ANDROID() || PERFETTO_BUILDFLAG_DEFINE_PERFETTO_OS_LINUX())
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_COMPONENT_BUILD() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_FORCE_DLOG_ON() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_FORCE_DLOG_OFF() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_FORCE_DCHECK_ON() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_FORCE_DCHECK_OFF() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_VERBOSE_LOGS() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_VERSION_GEN() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_TP_PERCENTILE() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_TP_LINENOISE() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_TP_HTTPD() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_TP_JSON() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_LOCAL_SYMBOLIZER() (PERFETTO_BUILDFLAG_DEFINE_PERFETTO_OS_LINUX() || PERFETTO_BUILDFLAG_DEFINE_PERFETTO_OS_MAC() ||PERFETTO_BUILDFLAG_DEFINE_PERFETTO_OS_WIN())
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_ZLIB() (1)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_TRACED_PERF() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_HEAPPROFD() (0)
#define PERFETTO_BUILDFLAG_DEFINE_PERFETTO_STDERR_CRASH_DUMP() (0)

// clang-format on
#endif  // GEN_BUILD_CONFIG_PERFETTO_BUILD_FLAGS_H_
