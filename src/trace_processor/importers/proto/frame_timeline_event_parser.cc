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

#include "src/trace_processor/importers/proto/frame_timeline_event_parser.h"

#include <inttypes.h>

#include "perfetto/ext/base/utils.h"
#include "perfetto/protozero/field.h"
#include "src/trace_processor/importers/common/args_tracker.h"
#include "src/trace_processor/importers/common/event_tracker.h"
#include "src/trace_processor/importers/common/process_tracker.h"
#include "src/trace_processor/importers/common/slice_tracker.h"
#include "src/trace_processor/importers/common/track_tracker.h"
#include "src/trace_processor/types/trace_processor_context.h"

#include "protos/perfetto/trace/interned_data/interned_data.pbzero.h"

namespace perfetto {
namespace trace_processor {

using ExpectedDisplayFrameStartDecoder =
    protos::pbzero::FrameTimelineEvent_ExpectedDisplayFrameStart_Decoder;
using ActualDisplayFrameStartDecoder =
    protos::pbzero::FrameTimelineEvent_ActualDisplayFrameStart_Decoder;

using ExpectedSurfaceFrameStartDecoder =
    protos::pbzero::FrameTimelineEvent_ExpectedSurfaceFrameStart_Decoder;
using ActualSurfaceFrameStartDecoder =
    protos::pbzero::FrameTimelineEvent_ActualSurfaceFrameStart_Decoder;

using FrameEndDecoder = protos::pbzero::FrameTimelineEvent_FrameEnd_Decoder;

static StringId JankTypeBitmaskToStringId(TraceProcessorContext* context,
                                          int32_t jank_type) {
  if (jank_type == FrameTimelineEvent::JANK_UNSPECIFIED)
    return context->storage->InternString("Unspecified");
  if (jank_type == FrameTimelineEvent::JANK_NONE)
    return context->storage->InternString("None");

  std::vector<std::string> jank_reasons;
  if (jank_type & FrameTimelineEvent::JANK_SF_SCHEDULING)
    jank_reasons.emplace_back("SurfaceFlinger Scheduling");
  if (jank_type & FrameTimelineEvent::JANK_PREDICTION_ERROR)
    jank_reasons.emplace_back("Prediction Error");
  if (jank_type & FrameTimelineEvent::JANK_DISPLAY_HAL)
    jank_reasons.emplace_back("Display HAL");
  if (jank_type & FrameTimelineEvent::JANK_SF_CPU_DEADLINE_MISSED)
    jank_reasons.emplace_back("SurfaceFlinger CPU Deadline Missed");
  if (jank_type & FrameTimelineEvent::JANK_SF_GPU_DEADLINE_MISSED)
    jank_reasons.emplace_back("SurfaceFlinger GPU Deadline Missed");
  if (jank_type & FrameTimelineEvent::JANK_APP_DEADLINE_MISSED)
    jank_reasons.emplace_back("App Deadline Missed");
  if (jank_type & FrameTimelineEvent::JANK_BUFFER_STUFFING)
    jank_reasons.emplace_back("Buffer Stuffing");
  if (jank_type & FrameTimelineEvent::JANK_UNKNOWN)
    jank_reasons.emplace_back("Unknown jank");

  std::string jank_str(
      std::accumulate(jank_reasons.begin(), jank_reasons.end(), std::string(),
                      [](const std::string& l, const std::string& r) {
                        return l.empty() ? r : l + ", " + r;
                      }));
  return context->storage->InternString(base::StringView(jank_str));
}

FrameTimelineEventParser::FrameTimelineEventParser(
    TraceProcessorContext* context)
    : context_(context),
      present_type_ids_{
          {context->storage->InternString(
               "Unspecified Present") /* PRESENT_UNSPECIFIED */,
           context->storage->InternString(
               "On-time Present") /* PRESENT_ON_TIME */,
           context->storage->InternString("Late Present") /* PRESENT_LATE */,
           context->storage->InternString("Early Present") /* PRESENT_EARLY */,
           context->storage->InternString(
               "Dropped Frame") /* PRESENT_DROPPED */}},
      expected_timeline_track_name_(
          context->storage->InternString("Expected Timeline")),
      actual_timeline_track_name_(
          context->storage->InternString("Actual Timeline")) {}

void FrameTimelineEventParser::ParseExpectedDisplayFrameStart(
    int64_t timestamp,
    ConstBytes bufferBlob) {
  ExpectedDisplayFrameStartDecoder event(bufferBlob.data, bufferBlob.size);
  if (!event.has_cookie()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_token()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_pid()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  int64_t cookie = event.cookie();
  int64_t token = event.token();
  StringId name_id =
      context_->storage->InternString(base::StringView(std::to_string(token)));

  UniquePid upid = context_->process_tracker->GetOrCreateProcess(
      static_cast<uint32_t>(event.pid()));
  auto expected_track_set_id =
      context_->async_track_set_tracker->InternFrameTimelineSet(
          upid, expected_timeline_track_name_);
  cookie_track_set_id_map_[cookie] = expected_track_set_id;

  tables::ExpectedFrameTimelineSliceTable::Row expected_row;
  expected_row.ts = timestamp;
  expected_row.track_id =
      context_->async_track_set_tracker->Begin(expected_track_set_id, cookie);
  expected_row.name = name_id;

  expected_row.display_frame_token = token;
  expected_row.upid = upid;
  context_->slice_tracker->BeginFrameTimeline(expected_row);
}

void FrameTimelineEventParser::ParseActualDisplayFrameStart(
    int64_t timestamp,
    ConstBytes bufferBlob) {
  ActualDisplayFrameStartDecoder event(bufferBlob.data, bufferBlob.size);
  if (!event.has_cookie()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_token()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_pid()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  int64_t cookie = event.cookie();
  int64_t token = event.token();
  StringId name_id =
      context_->storage->InternString(base::StringView(std::to_string(token)));

  UniquePid upid = context_->process_tracker->GetOrCreateProcess(
      static_cast<uint32_t>(event.pid()));
  auto actual_track_set_id =
      context_->async_track_set_tracker->InternFrameTimelineSet(
          upid, actual_timeline_track_name_);
  cookie_track_set_id_map_[cookie] = actual_track_set_id;

  tables::ActualFrameTimelineSliceTable::Row actual_row;
  actual_row.ts = timestamp;
  actual_row.track_id =
      context_->async_track_set_tracker->Begin(actual_track_set_id, cookie);
  actual_row.name = name_id;
  actual_row.display_frame_token = token;
  actual_row.upid = upid;
  actual_row.present_type =
      present_type_ids_[static_cast<size_t>(event.present_type())];
  actual_row.on_time_finish = event.on_time_finish();
  actual_row.gpu_composition = event.gpu_composition();
  actual_row.jank_type = JankTypeBitmaskToStringId(context_, event.jank_type());
  context_->slice_tracker->BeginFrameTimeline(actual_row);
}

void FrameTimelineEventParser::ParseExpectedSurfaceFrameStart(
    int64_t timestamp,
    ConstBytes bufferBlob) {
  ExpectedSurfaceFrameStartDecoder event(bufferBlob.data, bufferBlob.size);

  if (!event.has_cookie()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_token()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_display_frame_token()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_pid()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  int64_t cookie = event.cookie();
  int64_t token = event.token();
  int64_t display_frame_token = event.display_frame_token();
  UniquePid upid = context_->process_tracker->GetOrCreateProcess(
      static_cast<uint32_t>(event.pid()));
  auto token_set_it = expected_timeline_token_map_.find(upid);
  if (token_set_it != expected_timeline_token_map_.end()) {
    auto& token_set = token_set_it->second;
    if (token_set.find(token) != token_set.end()) {
      // If we already have an expected timeline for a token, the expectations
      // are same for all frames that use the token. No need to add duplicate
      // entries.
      return;
    }
  }
  // This is the first time we are seeing this token for this process. Add to
  // the map.
  expected_timeline_token_map_[upid].insert(token);

  StringId layer_name_id = event.has_layer_name()
                               ? context_->storage->InternString(
                                     base::StringView(event.layer_name()))
                               : kNullStringId;
  StringId name_id =
      context_->storage->InternString(base::StringView(std::to_string(token)));

  auto expected_track_set_id =
      context_->async_track_set_tracker->InternFrameTimelineSet(
          upid, expected_timeline_track_name_);
  cookie_track_set_id_map_[cookie] = expected_track_set_id;

  tables::ExpectedFrameTimelineSliceTable::Row expected_row;
  expected_row.ts = timestamp;
  expected_row.track_id =
      context_->async_track_set_tracker->Begin(expected_track_set_id, cookie);
  expected_row.name = name_id;

  expected_row.surface_frame_token = token;
  expected_row.display_frame_token = display_frame_token;
  expected_row.upid = upid;
  expected_row.layer_name = layer_name_id;
  context_->slice_tracker->BeginFrameTimeline(expected_row);
}

void FrameTimelineEventParser::ParseActualSurfaceFrameStart(
    int64_t timestamp,
    ConstBytes bufferBlob) {
  ActualSurfaceFrameStartDecoder event(bufferBlob.data, bufferBlob.size);

  if (!event.has_cookie()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_token()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_display_frame_token()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  if (!event.has_pid()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  int64_t cookie = event.cookie();
  int64_t token = event.token();
  int64_t display_frame_token = event.display_frame_token();
  UniquePid upid = context_->process_tracker->GetOrCreateProcess(
      static_cast<uint32_t>(event.pid()));
  StringId layer_name_id;
  if (event.has_layer_name())
    layer_name_id =
        context_->storage->InternString(base::StringView(event.layer_name()));
  StringId name_id =
      context_->storage->InternString(base::StringView(std::to_string(token)));

  auto actual_track_set_id =
      context_->async_track_set_tracker->InternFrameTimelineSet(
          upid, actual_timeline_track_name_);
  cookie_track_set_id_map_[cookie] = actual_track_set_id;

  tables::ActualFrameTimelineSliceTable::Row actual_row;
  actual_row.ts = timestamp;
  actual_row.track_id =
      context_->async_track_set_tracker->Begin(actual_track_set_id, cookie);
  actual_row.name = name_id;
  actual_row.surface_frame_token = token;
  actual_row.display_frame_token = display_frame_token;
  actual_row.upid = upid;
  actual_row.layer_name = layer_name_id;
  actual_row.present_type =
      present_type_ids_[static_cast<size_t>(event.present_type())];
  actual_row.on_time_finish = event.on_time_finish();
  actual_row.gpu_composition = event.gpu_composition();
  actual_row.jank_type = JankTypeBitmaskToStringId(context_, event.jank_type());
  context_->slice_tracker->BeginFrameTimeline(actual_row);
}

void FrameTimelineEventParser::ParseFrameEnd(int64_t timestamp,
                                             ConstBytes bufferBlob) {
  FrameEndDecoder event(bufferBlob.data, bufferBlob.size);

  if (!event.has_cookie()) {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
    return;
  }

  int64_t cookie = event.cookie();
  auto it = cookie_track_set_id_map_.find(cookie);
  if (it == cookie_track_set_id_map_.end())
    return;
  auto track_set_id = it->second;
  auto track_id = context_->async_track_set_tracker->End(track_set_id, cookie);
  context_->slice_tracker->EndFrameTimeline(timestamp, track_id);
  cookie_track_set_id_map_.erase(it);
}

void FrameTimelineEventParser::ParseFrameTimelineEvent(int64_t timestamp,
                                                       ConstBytes blob) {
  protos::pbzero::FrameTimelineEvent_Decoder frame_event(blob.data, blob.size);
  context_->storage->InternString(base::StringView(std::to_string(timestamp)));
  if (frame_event.has_expected_display_frame_start()) {
    ParseExpectedDisplayFrameStart(timestamp,
                                   frame_event.expected_display_frame_start());
  } else if (frame_event.has_actual_display_frame_start()) {
    ParseActualDisplayFrameStart(timestamp,
                                 frame_event.actual_display_frame_start());
  } else if (frame_event.has_expected_surface_frame_start()) {
    ParseExpectedSurfaceFrameStart(timestamp,
                                   frame_event.expected_surface_frame_start());
  } else if (frame_event.has_actual_surface_frame_start()) {
    ParseActualSurfaceFrameStart(timestamp,
                                 frame_event.actual_surface_frame_start());
  } else if (frame_event.has_frame_end()) {
    ParseFrameEnd(timestamp, frame_event.frame_end());
  } else {
    context_->storage->IncrementStats(
        stats::frame_timeline_event_parser_errors);
  }
}
}  // namespace trace_processor
}  // namespace perfetto
