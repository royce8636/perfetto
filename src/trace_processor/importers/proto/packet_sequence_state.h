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

#ifndef SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_PACKET_SEQUENCE_STATE_H_
#define SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_PACKET_SEQUENCE_STATE_H_

#include <stdint.h>

#include <unordered_map>
#include <vector>

#include "perfetto/base/compiler.h"
#include "perfetto/protozero/proto_decoder.h"
#include "src/trace_processor/stack_profile_tracker.h"
#include "src/trace_processor/trace_blob_view.h"
#include "src/trace_processor/trace_processor_context.h"
#include "src/trace_processor/trace_storage.h"

#include "protos/perfetto/trace/trace_packet_defaults.pbzero.h"

namespace perfetto {
namespace trace_processor {

#if PERFETTO_DCHECK_IS_ON()
// When called from GetOrCreateDecoder(), should include the stringified name of
// the MessageType.
#define PERFETTO_TYPE_IDENTIFIER PERFETTO_DEBUG_FUNCTION_IDENTIFIER()
#else  // PERFETTO_DCHECK_IS_ON()
#define PERFETTO_TYPE_IDENTIFIER nullptr
#endif  // PERFETTO_DCHECK_IS_ON()

class PacketSequenceState {
 public:
  // Entry in an interning index, refers to the interned message.
  class InternedMessageView {
   public:
    InternedMessageView(TraceBlobView msg) : message_(std::move(msg)) {}

    InternedMessageView(InternedMessageView&&) noexcept = default;
    InternedMessageView& operator=(InternedMessageView&&) = default;

    // Allow copy by cloning the TraceBlobView. This is required for
    // UpdateTracePacketDefaults().
    InternedMessageView(const InternedMessageView& view)
        : message_(view.message_.slice(0, view.message_.length())) {}
    InternedMessageView& operator=(const InternedMessageView& view) {
      this->message_ = view.message_.slice(0, view.message_.length());
      this->decoder_ = nullptr;
      this->decoder_type_ = nullptr;
      this->submessages_.clear();
      return *this;
    }

    // Lazily initializes and returns the decoder object for the message. The
    // decoder is stored in the InternedMessageView to avoid having to parse the
    // message multiple times.
    template <typename MessageType>
    typename MessageType::Decoder* GetOrCreateDecoder() {
      if (!decoder_) {
        // Lazy init the decoder and save it away, so that we don't have to
        // reparse the message every time we access the interning entry.
        decoder_ = std::unique_ptr<void, std::function<void(void*)>>(
            new
            typename MessageType::Decoder(message_.data(), message_.length()),
            [](void* obj) {
              delete reinterpret_cast<typename MessageType::Decoder*>(obj);
            });
        decoder_type_ = PERFETTO_TYPE_IDENTIFIER;
      }
      // Verify that the type of the decoder didn't change.
      if (PERFETTO_TYPE_IDENTIFIER &&
          strcmp(decoder_type_,
                 // GCC complains if this arg can be null.
                 PERFETTO_TYPE_IDENTIFIER ? PERFETTO_TYPE_IDENTIFIER : "") !=
              0) {
        PERFETTO_FATAL(
            "Interning entry accessed under different types! previous type: "
            "%s. new type: %s.",
            decoder_type_, __PRETTY_FUNCTION__);
      }
      return reinterpret_cast<typename MessageType::Decoder*>(decoder_.get());
    }

    // Lookup a submessage of the interned message, which is then itself stored
    // as InternedMessageView, so that we only need to parse it once. Returns
    // nullptr if the field isn't set.
    // TODO(eseckler): Support repeated fields.
    template <typename MessageType, uint32_t FieldId>
    InternedMessageView* GetOrCreateSubmessageView() {
      auto it = submessages_.find(FieldId);
      if (it != submessages_.end())
        return it->second.get();
      auto* decoder = GetOrCreateDecoder<MessageType>();
      // Calls the at() template method on the decoder.
      auto field = decoder->template at<FieldId>().as_bytes();
      if (!field.data)
        return nullptr;
      const size_t offset = message_.offset_of(field.data);
      TraceBlobView submessage = message_.slice(offset, field.size);
      InternedMessageView* submessage_view =
          new InternedMessageView(std::move(submessage));
      submessages_.emplace_hint(
          it, FieldId, std::unique_ptr<InternedMessageView>(submessage_view));
      return submessage_view;
    }

    const TraceBlobView& message() { return message_; }

   private:
    using SubMessageViewMap =
        std::unordered_map<uint32_t /*field_id*/,
                           std::unique_ptr<InternedMessageView>>;

    TraceBlobView message_;

    // Stores the decoder for the message_, so that the message does not have to
    // be re-decoded every time the interned message is looked up. Lazily
    // initialized in GetOrCreateDecoder(). Since we don't know the type of the
    // decoder until GetOrCreateDecoder() is called, we store the decoder as a
    // void* unique_pointer with a destructor function that's supplied in
    // GetOrCreateDecoder() when the decoder is created.
    std::unique_ptr<void, std::function<void(void*)>> decoder_;

    // Type identifier for the decoder. Only valid in debug builds and on
    // supported platforms. Used to verify that GetOrCreateDecoder() is always
    // called with the same template argument.
    const char* decoder_type_ = nullptr;

    // Views of submessages of the interned message. Submessages are lazily
    // added by GetOrCreateSubmessageView(). By storing submessages and their
    // decoders, we avoid having to decode submessages multiple times if they
    // looked up often.
    SubMessageViewMap submessages_;
  };

  using InternedMessageMap =
      std::unordered_map<uint64_t /*iid*/, InternedMessageView>;
  using InternedFieldMap =
      std::unordered_map<uint32_t /*field_id*/, InternedMessageMap>;

  struct GenerationData {
    InternedFieldMap interned_data;
    base::Optional<InternedMessageView> trace_packet_defaults;
  };

  // TODO(eseckler): Reference count the generations so that we can get rid of
  // past generations once all packets referring to them have been parsed.
  using GenerationList = std::vector<GenerationData>;

  PacketSequenceState(TraceProcessorContext* context)
      : context_(context), stack_profile_tracker_(context) {
    generations_.emplace_back();
  }

  int64_t IncrementAndGetTrackEventTimeNs(int64_t delta_ns) {
    PERFETTO_DCHECK(track_event_timestamps_valid());
    track_event_timestamp_ns_ += delta_ns;
    return track_event_timestamp_ns_;
  }

  int64_t IncrementAndGetTrackEventThreadTimeNs(int64_t delta_ns) {
    PERFETTO_DCHECK(track_event_timestamps_valid());
    track_event_thread_timestamp_ns_ += delta_ns;
    return track_event_thread_timestamp_ns_;
  }

  int64_t IncrementAndGetTrackEventThreadInstructionCount(int64_t delta) {
    PERFETTO_DCHECK(track_event_timestamps_valid());
    track_event_thread_instruction_count_ += delta;
    return track_event_thread_instruction_count_;
  }

  void OnPacketLoss() {
    packet_loss_ = true;
    track_event_timestamps_valid_ = false;
  }

  void OnIncrementalStateCleared() {
    packet_loss_ = false;
    generations_.emplace_back();  // Bump generation number
  }

  void SetThreadDescriptor(int32_t pid,
                           int32_t tid,
                           int64_t timestamp_ns,
                           int64_t thread_timestamp_ns,
                           int64_t thread_instruction_count) {
    track_event_timestamps_valid_ = true;
    pid_and_tid_valid_ = true;
    pid_ = pid;
    tid_ = tid;
    track_event_timestamp_ns_ = timestamp_ns;
    track_event_thread_timestamp_ns_ = thread_timestamp_ns;
    track_event_thread_instruction_count_ = thread_instruction_count;
  }

  bool IsIncrementalStateValid() const { return !packet_loss_; }

  StackProfileTracker& stack_profile_tracker() {
    return stack_profile_tracker_;
  }

  // Returns the index of the current generation in the GenerationList.
  size_t current_generation() const { return generations_.size() - 1; }

  bool track_event_timestamps_valid() const {
    return track_event_timestamps_valid_;
  }

  bool pid_and_tid_valid() const { return pid_and_tid_valid_; }

  int32_t pid() const { return pid_; }
  int32_t tid() const { return tid_; }

  void InternMessage(uint32_t field_id, TraceBlobView message) {
    constexpr auto kIidFieldNumber = 1;

    uint64_t iid = 0;
    auto message_start = message.data();
    auto message_size = message.length();
    protozero::ProtoDecoder decoder(message_start, message_size);

    auto field = decoder.FindField(kIidFieldNumber);
    if (PERFETTO_UNLIKELY(!field)) {
      PERFETTO_DLOG("Interned message without interning_id");
      context_->storage->IncrementStats(stats::interned_data_tokenizer_errors);
      return;
    }
    iid = field.as_uint64();

    auto* map = &generations_.back().interned_data[field_id];
    auto res = map->emplace(iid, InternedMessageView(std::move(message)));

    // If a message with this ID is already interned in the same generation,
    // its data should not have changed (this is forbidden by the InternedData
    // proto).
    // TODO(eseckler): This DCHECK assumes that the message is encoded the
    // same way if it is re-emitted.
    PERFETTO_DCHECK(res.second ||
                    (res.first->second.message().length() == message_size &&
                     memcmp(res.first->second.message().data(), message_start,
                            message_size) == 0));
  }

  // Returns |nullptr| if the message with the given |iid| was not found (also
  // records a stat in this case).
  template <uint32_t FieldId, typename MessageType>
  typename MessageType::Decoder* LookupInternedMessage(size_t generation,
                                                       uint64_t iid) {
    PERFETTO_CHECK(generation <= generations_.size());
    auto* field_map = &generations_[generation].interned_data;
    auto field_it = field_map->find(FieldId);
    if (field_it != field_map->end()) {
      auto* message_map = &field_it->second;
      auto it = message_map->find(iid);
      if (it != message_map->end()) {
        return it->second.GetOrCreateDecoder<MessageType>();
      }
    }
    context_->storage->IncrementStats(stats::interned_data_tokenizer_errors);
    PERFETTO_DLOG("Could not find interning entry for field ID %" PRIu32
                  ", generation %zu, and IID %" PRIu64,
                  FieldId, generation, iid);
    return nullptr;
  }

  void UpdateTracePacketDefaults(TraceBlobView trace_packet_defaults) {
    if (generations_.back().trace_packet_defaults) {
      // The new defaults should only apply to subsequent messages on the
      // sequence. Add a new generation with the updated defaults but the
      // current generation's interned data state.
      const InternedFieldMap& current_interned_data =
          generations_.back().interned_data;
      generations_.emplace_back();
      generations_.back().interned_data = current_interned_data;
    }
    generations_.back().trace_packet_defaults =
        InternedMessageView(std::move(trace_packet_defaults));
  }

  // Returns |nullptr| if no defaults were set in the given generation.
  InternedMessageView* GetTracePacketDefaultsView(size_t generation) {
    PERFETTO_CHECK(generation <= generations_.size());
    if (!generations_[generation].trace_packet_defaults)
      return nullptr;
    return &generations_[generation].trace_packet_defaults.value();
  }

  // Returns |nullptr| if no defaults were set in the given generation.
  typename protos::pbzero::TracePacketDefaults::Decoder* GetTracePacketDefaults(
      size_t generation) {
    InternedMessageView* view = GetTracePacketDefaultsView(generation);
    if (!view)
      return nullptr;
    return view->GetOrCreateDecoder<protos::pbzero::TracePacketDefaults>();
  }

 private:
  TraceProcessorContext* context_;

  // If true, incremental state on the sequence is considered invalid until we
  // see the next packet with incremental_state_cleared. We assume that we
  // missed some packets at the beginning of the trace.
  bool packet_loss_ = true;

  // We can only consider TrackEvent delta timestamps to be correct after we
  // have observed a thread descriptor (since the last packet loss).
  bool track_event_timestamps_valid_ = false;

  // |pid_| and |tid_| are only valid after we parsed at least one
  // ThreadDescriptor packet on the sequence.
  bool pid_and_tid_valid_ = false;

  // Process/thread ID of the packet sequence set by a ThreadDescriptor
  // packet. Used as default values for TrackEvents that don't specify a
  // pid/tid override. Only valid after |pid_and_tid_valid_| is set to true.
  int32_t pid_ = 0;
  int32_t tid_ = 0;

  // Current wall/thread timestamps/counters used as reference for the next
  // TrackEvent delta timestamp.
  int64_t track_event_timestamp_ns_ = 0;
  int64_t track_event_thread_timestamp_ns_ = 0;
  int64_t track_event_thread_instruction_count_ = 0;

  GenerationList generations_;
  StackProfileTracker stack_profile_tracker_;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_PACKET_SEQUENCE_STATE_H_
