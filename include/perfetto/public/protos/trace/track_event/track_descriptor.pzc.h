/*
 * Copyright (C) 2023 The Android Open Source Project
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

#ifndef INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_TRACK_DESCRIPTOR_PZC_H_
#define INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_TRACK_DESCRIPTOR_PZC_H_

#include <stdbool.h>
#include <stdint.h>

#include "perfetto/public/pb_macros.h"

PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeProcessDescriptor);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeThreadDescriptor);
PERFETTO_PB_MSG_DECL(perfetto_protos_CounterDescriptor);
PERFETTO_PB_MSG_DECL(perfetto_protos_ProcessDescriptor);
PERFETTO_PB_MSG_DECL(perfetto_protos_ThreadDescriptor);

PERFETTO_PB_MSG(perfetto_protos_TrackDescriptor);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor, VARINT, uint64_t, uuid, 1);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor,
                  VARINT,
                  uint64_t,
                  parent_uuid,
                  5);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor,
                  STRING,
                  const char*,
                  name,
                  2);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor,
                  MSG,
                  perfetto_protos_ProcessDescriptor,
                  process,
                  3);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor,
                  MSG,
                  perfetto_protos_ChromeProcessDescriptor,
                  chrome_process,
                  6);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor,
                  MSG,
                  perfetto_protos_ThreadDescriptor,
                  thread,
                  4);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor,
                  MSG,
                  perfetto_protos_ChromeThreadDescriptor,
                  chrome_thread,
                  7);
PERFETTO_PB_FIELD(perfetto_protos_TrackDescriptor,
                  MSG,
                  perfetto_protos_CounterDescriptor,
                  counter,
                  8);

#endif  // INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_TRACK_DESCRIPTOR_PZC_H_
