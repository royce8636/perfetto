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
#include "src/trace_processor/importers/proto/track_event_module.h"

#include "perfetto/base/build_config.h"
#include "perfetto/base/logging.h"
#include "perfetto/ext/base/string_utils.h"
#include "src/trace_processor/importers/common/track_tracker.h"
#include "src/trace_processor/importers/proto/track_event_tracker.h"
#include "src/trace_processor/types/trace_processor_context.h"

#include "protos/perfetto/config/data_source_config.pbzero.h"
#include "protos/perfetto/config/trace_config.pbzero.h"
#include "protos/perfetto/trace/trace_packet.pbzero.h"

namespace perfetto {
namespace trace_processor {

using perfetto::protos::pbzero::TracePacket;

TrackEventModule::TrackEventModule(TraceProcessorContext* context)
    : track_event_tracker_(new TrackEventTracker(context)),
      tokenizer_(context, track_event_tracker_.get()),
      parser_(context, track_event_tracker_.get()) {
  RegisterForField(TracePacket::kTrackEventFieldNumber, context);
  RegisterForField(TracePacket::kTrackDescriptorFieldNumber, context);
  RegisterForField(TracePacket::kThreadDescriptorFieldNumber, context);
  RegisterForField(TracePacket::kProcessDescriptorFieldNumber, context);
}

TrackEventModule::~TrackEventModule() = default;

ModuleResult TrackEventModule::TokenizePacket(
    const TracePacket::Decoder& decoder,
    TraceBlobView* packet,
    int64_t packet_timestamp,
    PacketSequenceState* state,
    uint32_t field_id) {
  switch (field_id) {
    case TracePacket::kTrackDescriptorFieldNumber:
      return tokenizer_.TokenizeTrackDescriptorPacket(state, decoder,
                                                      packet_timestamp);
    case TracePacket::kTrackEventFieldNumber:
      tokenizer_.TokenizeTrackEventPacket(state, decoder, packet,
                                          packet_timestamp);
      return ModuleResult::Handled();
    case TracePacket::kThreadDescriptorFieldNumber:
      // TODO(eseckler): Remove once Chrome has switched to TrackDescriptors.
      return tokenizer_.TokenizeThreadDescriptorPacket(state, decoder);
  }
  return ModuleResult::Ignored();
}

void TrackEventModule::ParseTrackEventData(const TracePacket::Decoder& decoder,
                                           int64_t ts,
                                           const TrackEventData& data) {
  parser_.ParseTrackEvent(ts, &data, decoder.track_event(),
                          decoder.trusted_packet_sequence_id());
}

void TrackEventModule::ParseTracePacketData(const TracePacket::Decoder& decoder,
                                            int64_t ts,
                                            const TracePacketData&,
                                            uint32_t field_id) {
  switch (field_id) {
    case TracePacket::kTrackDescriptorFieldNumber:
      parser_.ParseTrackDescriptor(ts, decoder.track_descriptor(),
                                   decoder.trusted_packet_sequence_id());
      break;
    case TracePacket::kProcessDescriptorFieldNumber:
      // TODO(eseckler): Remove once Chrome has switched to TrackDescriptors.
      parser_.ParseProcessDescriptor(ts, decoder.process_descriptor());
      break;
    case TracePacket::kThreadDescriptorFieldNumber:
      // TODO(eseckler): Remove once Chrome has switched to TrackDescriptors.
      parser_.ParseThreadDescriptor(decoder.thread_descriptor());
      break;
    case TracePacket::kTrackEventFieldNumber:
      PERFETTO_DFATAL("Wrong TracePacket number");
  }
}

void TrackEventModule::OnIncrementalStateCleared(uint32_t packet_sequence_id) {
  track_event_tracker_->OnIncrementalStateCleared(packet_sequence_id);
}

void TrackEventModule::OnFirstPacketOnSequence(uint32_t packet_sequence_id) {
  track_event_tracker_->OnFirstPacketOnSequence(packet_sequence_id);
}

void TrackEventModule::NotifyEndOfFile() {
  parser_.NotifyEndOfFile();
}

}  // namespace trace_processor
}  // namespace perfetto
