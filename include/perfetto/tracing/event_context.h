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

#ifndef INCLUDE_PERFETTO_TRACING_EVENT_CONTEXT_H_
#define INCLUDE_PERFETTO_TRACING_EVENT_CONTEXT_H_

#include "perfetto/protozero/message_handle.h"
#include "perfetto/tracing/internal/track_event_internal.h"
#include "perfetto/tracing/traced_proto.h"
#include "protos/perfetto/trace/trace_packet.pbzero.h"

namespace perfetto {
namespace protos {
namespace pbzero {
class DebugAnnotation;
}  // namespace pbzero
}  // namespace protos

namespace internal {
class TrackEventInternal;
}

// Allows adding custom arguments into track events. Example:
//
//   TRACE_EVENT_BEGIN("category", "Title",
//                     [](perfetto::EventContext ctx) {
//                       auto* log = ctx.event()->set_log_message();
//                       log->set_body_iid(1234);
//
//                       ctx.AddDebugAnnotation("name", 1234);
//                     });
//
class PERFETTO_EXPORT EventContext {
 public:
  EventContext(EventContext&&) = default;

  // For Chromium during the transition phase to the client library.
  // TODO(eseckler): Remove once Chromium has switched to client lib entirely.
  explicit EventContext(
      protos::pbzero::TrackEvent* event,
      internal::TrackEventIncrementalState* incremental_state = nullptr)
      : event_(event), incremental_state_(incremental_state) {}

  ~EventContext();

  internal::TrackEventIncrementalState* GetIncrementalState() const {
    return incremental_state_;
  }

  // Get a TrackEvent message to write typed arguments to.
  //
  // event() is a template method to allow callers to specify a subclass of
  // TrackEvent instead. Those subclasses correspond to TrackEvent message with
  // application-specific extensions. More information in
  // design-docs/extensions.md.
  template <typename EventType = protos::pbzero::TrackEvent>
  EventType* event() const {
    // As the method does downcasting, we check that a target subclass does
    // not add new fields.
    static_assert(
        sizeof(EventType) == sizeof(protos::pbzero::TrackEvent),
        "Event type must be binary-compatible with protos::pbzero::TrackEvent");
    return static_cast<EventType*>(event_);
  }

  // Convert a raw pointer to protozero message to TracedProto which captures
  // the reference to this EventContext.
  template <typename MessageType>
  TracedProto<MessageType> Wrap(MessageType* message) {
    static_assert(std::is_base_of<protozero::Message, MessageType>::value,
                  "TracedProto can be used only with protozero messages");

    return TracedProto<MessageType>(message, this);
  }

  // Add a new `debug_annotation` proto message and populate it from |value|
  // using perfetto::TracedValue API. Users should generally prefer passing
  // values directly to TRACE_EVENT (i.e. TRACE_EVENT(..., "arg", value, ...);)
  // but in rare cases (e.g. when an argument should be written conditionally)
  // EventContext::AddDebugAnnotation provides an explicit equivalent.
  template <typename T>
  void AddDebugAnnotation(const char* name, T&& value) {
    auto annotation = AddDebugAnnotation(name);
    WriteIntoTracedValue(internal::CreateTracedValueFromProto(annotation, this),
                         std::forward<T>(value));
  }

 private:
  template <typename, size_t, typename, typename>
  friend class TrackEventInternedDataIndex;
  friend class internal::TrackEventInternal;

  using TracePacketHandle =
      ::protozero::MessageHandle<protos::pbzero::TracePacket>;

  EventContext(TracePacketHandle, internal::TrackEventIncrementalState*);
  EventContext(const EventContext&) = delete;

  protos::pbzero::DebugAnnotation* AddDebugAnnotation(const char* name);

  TracePacketHandle trace_packet_;
  protos::pbzero::TrackEvent* event_;
  internal::TrackEventIncrementalState* incremental_state_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_EVENT_CONTEXT_H_
