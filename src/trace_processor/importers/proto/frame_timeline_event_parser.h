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

#ifndef SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_FRAME_TIMELINE_EVENT_PARSER_H_
#define SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_FRAME_TIMELINE_EVENT_PARSER_H_

#include "perfetto/protozero/field.h"
#include "src/trace_processor/importers/common/args_tracker.h"
#include "src/trace_processor/importers/proto/async_track_set_tracker.h"
#include "src/trace_processor/importers/proto/proto_incremental_state.h"
#include "src/trace_processor/storage/trace_storage.h"

#include "protos/perfetto/trace/android/frame_timeline_event.pbzero.h"

#include <map>
#include <unordered_map>
#include <unordered_set>

namespace perfetto {

namespace trace_processor {

using FrameTimelineEvent = protos::pbzero::FrameTimelineEvent;
using FrameTimelineEventDecoder = protos::pbzero::FrameTimelineEvent_Decoder;

class TraceProcessorContext;

// Class for parsing graphics frame related events.
class FrameTimelineEventParser {
 public:
  using ConstBytes = protozero::ConstBytes;
  using TrackSetId = AsyncTrackSetTracker::TrackSetId;
  explicit FrameTimelineEventParser(TraceProcessorContext*);

  void ParseFrameTimelineEvent(int64_t timestamp, ConstBytes);

 private:
  void ParseExpectedDisplayFrameStart(int64_t timestamp, ConstBytes);
  void ParseActualDisplayFrameStart(int64_t timestamp, ConstBytes);

  void ParseExpectedSurfaceFrameStart(int64_t timestamp, ConstBytes);
  void ParseActualSurfaceFrameStart(int64_t timestamp, ConstBytes);

  void ParseFrameEnd(int64_t timestamp, ConstBytes);

  TraceProcessorContext* const context_;
  // Cookie -> TrackSetId map. Since cookies are globally unique per slice, this
  // helps in allowing the producer to send only the cookie as the End marker
  // without the need for any other fields. The TrackSets could be interned
  // based on any number of fields in the Start marker but the global uniqueness
  // of the cookie makes it so that we can end a slice with just the cookie and
  // the TrackSetId.
  std::map<int64_t, TrackSetId> cookie_track_set_id_map_;
  std::array<StringId, 5> present_type_ids_;
  StringId expected_timeline_track_name_;
  StringId actual_timeline_track_name_;

  StringId surface_frame_token_id_;
  StringId display_frame_token_id_;
  StringId present_type_id_;
  StringId on_time_finish_id_;
  StringId gpu_composition_id_;
  StringId jank_type_id_;
  StringId layer_name_id_;

  // upid -> set of tokens map. The expected timeline is the same for a given
  // token no matter how many times its seen. We can safely ignore duplicates
  // for the expected timeline slices by caching the set of tokens seen so far
  // per upid. upid is used as a dimension here because we show the timeline
  // tracks for every process group.
  // This map is used only for SurfaceFrames because there is no way two
  // DisplayFrames use the same token unless there is something wrong with
  // SurfaceFlinger.
  std::unordered_map<UniquePid, std::unordered_set<int64_t>>
      expected_timeline_token_map_;

  std::multimap<int64_t, SliceId> display_token_to_surface_slice_;
};
}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_FRAME_TIMELINE_EVENT_PARSER_H_
