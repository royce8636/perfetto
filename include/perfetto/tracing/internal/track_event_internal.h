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

#ifndef INCLUDE_PERFETTO_TRACING_INTERNAL_TRACK_EVENT_INTERNAL_H_
#define INCLUDE_PERFETTO_TRACING_INTERNAL_TRACK_EVENT_INTERNAL_H_

#include "perfetto/tracing/trace_writer_base.h"
#include "protos/perfetto/trace/track_event/track_event.pbzero.h"

#include <unordered_map>

namespace perfetto {
class DataSourceConfig;
class DataSourceDescriptor;
class TrackEventContext;

namespace internal {
class TrackEventCategoryRegistry;

struct TrackEventIncrementalState {
  bool was_cleared = true;

  // Interned data.
  // TODO(skyostil): Replace this with something more clever that supports
  // dynamic strings too.
  std::unordered_map<const char*, uint64_t> event_names;
  std::unordered_map<const char*, uint64_t> categories;
};

// The backend portion of the track event trace point implemention. Outlined to
// a separate .cc file so it can be shared by different track event category
// namespaces.
class TrackEventInternal {
 public:
  static bool Initialize(
      bool (*register_data_source)(const DataSourceDescriptor&));

  static void EnableTracing(const TrackEventCategoryRegistry& registry,
                            const DataSourceConfig& config,
                            uint32_t instance_index);
  static void DisableTracing(const TrackEventCategoryRegistry& registry,
                             uint32_t instance_index);

  static perfetto::TrackEventContext WriteEvent(
      TraceWriterBase*,
      TrackEventIncrementalState*,
      const char* category,
      const char* name,
      perfetto::protos::pbzero::TrackEvent::Type);
};

}  // namespace internal
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_INTERNAL_TRACK_EVENT_INTERNAL_H_
