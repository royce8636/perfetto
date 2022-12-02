/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <numeric>
#include <utility>

#include "perfetto/ext/base/string_utils.h"
#include "src/trace_processor/importers/proto/content_analyzer.h"
#include "src/trace_processor/storage/trace_storage.h"

#include "src/trace_processor/importers/proto/trace.descriptor.h"

namespace perfetto {
namespace trace_processor {

ContentAnalyzerModule::ContentAnalyzerModule(TraceProcessorContext* context)
    : context_(context),
      pool_([]() {
        DescriptorPool pool;
        base::Status status = pool.AddFromFileDescriptorSet(
            kTraceDescriptor.data(), kTraceDescriptor.size());
        if (!status.ok()) {
          PERFETTO_ELOG("Could not add TracePacket proto descriptor %s",
                        status.c_message());
        }
        return pool;
      }()),
      computer_(&pool_, ".perfetto.protos.TracePacket") {
  RegisterForAllFields(context_);
}

ModuleResult ContentAnalyzerModule::TokenizePacket(
    const protos::pbzero::TracePacket_Decoder&,
    TraceBlobView* packet,
    int64_t /*packet_timestamp*/,
    PacketSequenceState*,
    uint32_t /*field_id*/) {
  computer_.Reset(packet->data(), packet->length());
  for (auto sample = computer_.GetNext(); sample.has_value();
       sample = computer_.GetNext()) {
    auto* value = aggregated_samples_.Find(computer_.GetPath());
    if (value)
      *value += *sample;
    else
      aggregated_samples_.Insert(computer_.GetPath(), *sample);
  }
  return ModuleResult::Ignored();
}

void ContentAnalyzerModule::NotifyEndOfFile() {
  // TODO(kraskevich): consider generating a flamegraph-compatable table once
  // Perfetto UI supports custom flamegraphs (b/227644078).
  for (auto sample = aggregated_samples_.GetIterator(); sample; ++sample) {
    std::string path_string;
    for (const auto& field : sample.key()) {
      if (field.has_field_name()) {
        if (!path_string.empty()) {
          path_string += '.';
        }
        path_string.append(field.field_name());
      }
      if (!path_string.empty()) {
        path_string += '.';
      }
      path_string.append(field.type_name());
    }
    tables::ExperimentalProtoContentTable::Row row;
    row.path = context_->storage->InternString(base::StringView(path_string));
    row.total_size = static_cast<int64_t>(sample.value());
    context_->storage->mutable_experimental_proto_content_table()->Insert(row);
  }
  aggregated_samples_.Clear();
}

}  // namespace trace_processor
}  // namespace perfetto
