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

#ifndef INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_TRACK_EVENT_PZC_H_
#define INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_TRACK_EVENT_PZC_H_

#include <stdbool.h>
#include <stdint.h>

#include "perfetto/public/pb_macros.h"

PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeActiveProcesses);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeApplicationStateInfo);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeCompositorSchedulerState);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeContentSettingsEventInfo);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeFrameReporter);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeHistogramSample);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeKeyedService);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeLatencyInfo);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeLegacyIpc);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeMessagePump);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeMojoEventInfo);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeRendererSchedulerState);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeUserEvent);
PERFETTO_PB_MSG_DECL(perfetto_protos_ChromeWindowHandleEventInfo);
PERFETTO_PB_MSG_DECL(perfetto_protos_DebugAnnotation);
PERFETTO_PB_MSG_DECL(perfetto_protos_LogMessage);
PERFETTO_PB_MSG_DECL(perfetto_protos_SourceLocation);
PERFETTO_PB_MSG_DECL(perfetto_protos_TaskExecution);
PERFETTO_PB_MSG_DECL(perfetto_protos_TrackEvent_LegacyEvent);

PERFETTO_PB_ENUM_IN_MSG(perfetto_protos_TrackEvent, Type){
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent,
                                  TYPE_UNSPECIFIED) = 0,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent,
                                  TYPE_SLICE_BEGIN) = 1,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent,
                                  TYPE_SLICE_END) = 2,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent, TYPE_INSTANT) = 3,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent, TYPE_COUNTER) = 4,
};

PERFETTO_PB_ENUM_IN_MSG(perfetto_protos_TrackEvent_LegacyEvent, FlowDirection){
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  FLOW_UNSPECIFIED) = 0,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  FLOW_IN) = 1,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  FLOW_OUT) = 2,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  FLOW_INOUT) = 3,
};

PERFETTO_PB_ENUM_IN_MSG(perfetto_protos_TrackEvent_LegacyEvent,
                        InstantEventScope){
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  SCOPE_UNSPECIFIED) = 0,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  SCOPE_GLOBAL) = 1,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  SCOPE_PROCESS) = 2,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_TrackEvent_LegacyEvent,
                                  SCOPE_THREAD) = 3,
};

PERFETTO_PB_MSG(perfetto_protos_EventName);
PERFETTO_PB_FIELD(perfetto_protos_EventName, VARINT, uint64_t, iid, 1);
PERFETTO_PB_FIELD(perfetto_protos_EventName, STRING, const char*, name, 2);

PERFETTO_PB_MSG(perfetto_protos_EventCategory);
PERFETTO_PB_FIELD(perfetto_protos_EventCategory, VARINT, uint64_t, iid, 1);
PERFETTO_PB_FIELD(perfetto_protos_EventCategory, STRING, const char*, name, 2);

PERFETTO_PB_MSG(perfetto_protos_TrackEventDefaults);
PERFETTO_PB_FIELD(perfetto_protos_TrackEventDefaults,
                  VARINT,
                  uint64_t,
                  track_uuid,
                  11);
PERFETTO_PB_FIELD(perfetto_protos_TrackEventDefaults,
                  VARINT,
                  uint64_t,
                  extra_counter_track_uuids,
                  31);
PERFETTO_PB_FIELD(perfetto_protos_TrackEventDefaults,
                  VARINT,
                  uint64_t,
                  extra_double_counter_track_uuids,
                  45);

PERFETTO_PB_MSG(perfetto_protos_TrackEvent);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  uint64_t,
                  category_iids,
                  3);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  STRING,
                  const char*,
                  categories,
                  22);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent, VARINT, uint64_t, name_iid, 10);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent, STRING, const char*, name, 23);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  enum perfetto_protos_TrackEvent_Type,
                  type,
                  9);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent, VARINT, uint64_t, track_uuid, 11);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  counter_value,
                  30);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  FIXED64,
                  double,
                  double_counter_value,
                  44);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  uint64_t,
                  extra_counter_track_uuids,
                  31);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  extra_counter_values,
                  12);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  uint64_t,
                  extra_double_counter_track_uuids,
                  45);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  FIXED64,
                  double,
                  extra_double_counter_values,
                  46);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  uint64_t,
                  flow_ids_old,
                  36);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent, FIXED64, uint64_t, flow_ids, 47);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  uint64_t,
                  terminating_flow_ids_old,
                  42);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  FIXED64,
                  uint64_t,
                  terminating_flow_ids,
                  48);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_DebugAnnotation,
                  debug_annotations,
                  4);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_TaskExecution,
                  task_execution,
                  5);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_LogMessage,
                  log_message,
                  21);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeCompositorSchedulerState,
                  cc_scheduler_state,
                  24);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeUserEvent,
                  chrome_user_event,
                  25);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeKeyedService,
                  chrome_keyed_service,
                  26);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeLegacyIpc,
                  chrome_legacy_ipc,
                  27);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeHistogramSample,
                  chrome_histogram_sample,
                  28);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeLatencyInfo,
                  chrome_latency_info,
                  29);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeFrameReporter,
                  chrome_frame_reporter,
                  32);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeApplicationStateInfo,
                  chrome_application_state_info,
                  39);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeRendererSchedulerState,
                  chrome_renderer_scheduler_state,
                  40);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeWindowHandleEventInfo,
                  chrome_window_handle_event_info,
                  41);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeContentSettingsEventInfo,
                  chrome_content_settings_event_info,
                  43);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeActiveProcesses,
                  chrome_active_processes,
                  49);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_SourceLocation,
                  source_location,
                  33);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  uint64_t,
                  source_location_iid,
                  34);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeMessagePump,
                  chrome_message_pump,
                  35);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_ChromeMojoEventInfo,
                  chrome_mojo_event_info,
                  38);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  timestamp_delta_us,
                  1);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  timestamp_absolute_us,
                  16);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  thread_time_delta_us,
                  2);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  thread_time_absolute_us,
                  17);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  thread_instruction_count_delta,
                  8);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  VARINT,
                  int64_t,
                  thread_instruction_count_absolute,
                  20);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent,
                  MSG,
                  perfetto_protos_TrackEvent_LegacyEvent,
                  legacy_event,
                  6);

PERFETTO_PB_MSG(perfetto_protos_TrackEvent_LegacyEvent);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  uint64_t,
                  name_iid,
                  1);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  int32_t,
                  phase,
                  2);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  int64_t,
                  duration_us,
                  3);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  int64_t,
                  thread_duration_us,
                  4);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  int64_t,
                  thread_instruction_delta,
                  15);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  uint64_t,
                  unscoped_id,
                  6);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  uint64_t,
                  local_id,
                  10);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  uint64_t,
                  global_id,
                  11);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  STRING,
                  const char*,
                  id_scope,
                  7);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  bool,
                  use_async_tts,
                  9);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  uint64_t,
                  bind_id,
                  8);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  bool,
                  bind_to_enclosing,
                  12);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  enum perfetto_protos_TrackEvent_LegacyEvent_FlowDirection,
                  flow_direction,
                  13);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  enum perfetto_protos_TrackEvent_LegacyEvent_InstantEventScope,
                  instant_event_scope,
                  14);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  int32_t,
                  pid_override,
                  18);
PERFETTO_PB_FIELD(perfetto_protos_TrackEvent_LegacyEvent,
                  VARINT,
                  int32_t,
                  tid_override,
                  19);

#endif  // INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_TRACK_EVENT_PZC_H_
