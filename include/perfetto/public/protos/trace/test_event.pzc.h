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

#ifndef INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TEST_EVENT_PZC_H_
#define INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TEST_EVENT_PZC_H_

#include <stdbool.h>
#include <stdint.h>

#include "perfetto/public/pb_macros.h"

PERFETTO_PB_MSG_DECL(perfetto_protos_DebugAnnotation);
PERFETTO_PB_MSG_DECL(perfetto_protos_TestEvent_TestPayload);

PERFETTO_PB_MSG(perfetto_protos_TestEvent);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent, STRING, const char*, str, 1);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent, VARINT, uint32_t, seq_value, 2);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent, VARINT, uint64_t, counter, 3);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent, VARINT, bool, is_last, 4);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent,
                  MSG,
                  perfetto_protos_TestEvent_TestPayload,
                  payload,
                  5);

PERFETTO_PB_MSG(perfetto_protos_TestEvent_TestPayload);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent_TestPayload,
                  STRING,
                  const char*,
                  str,
                  1);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent_TestPayload,
                  MSG,
                  perfetto_protos_TestEvent_TestPayload,
                  nested,
                  2);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent_TestPayload,
                  STRING,
                  const char*,
                  single_string,
                  4);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent_TestPayload,
                  VARINT,
                  int32_t,
                  single_int,
                  5);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent_TestPayload,
                  VARINT,
                  int32_t,
                  repeated_ints,
                  6);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent_TestPayload,
                  VARINT,
                  uint32_t,
                  remaining_nesting_depth,
                  3);
PERFETTO_PB_FIELD(perfetto_protos_TestEvent_TestPayload,
                  MSG,
                  perfetto_protos_DebugAnnotation,
                  debug_annotations,
                  7);

#endif  // INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TEST_EVENT_PZC_H_
