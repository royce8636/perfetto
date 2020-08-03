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

#ifndef SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_ARGS_TABLE_UTILS_H_
#define SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_ARGS_TABLE_UTILS_H_

#include "perfetto/protozero/proto_decoder.h"
#include "perfetto/trace_processor/status.h"
#include "src/trace_processor/importers/common/args_tracker.h"
#include "src/trace_processor/importers/proto/packet_sequence_state.h"
#include "src/trace_processor/storage/trace_storage.h"
#include "src/trace_processor/util/descriptors.h"

namespace perfetto {
namespace trace_processor {

// ProtoToArgsTable encapsulates the process of taking an arbitrary proto and
// associating each field as a column in an args set. This is done by traversing
// the proto using reflection (with descriptors provided by
// AddProtoFileDescriptor()) and creating column names equal to this traversal.
//
// I.E. given a proto like
//
// package perfetto.protos;
// message SubMessage {
//   optional int32 field = 1;
// }
// message MainMessage {
//   optional int32 field1 = 1;
//   optional string field2 = 2;
//   optional SubMessage field3 = 3;
// }
//
// We will get the args set columns "field1", "field2", "field3.field" and will
// store the values found inside as the result.
//
// Usage of this is as follows:
//
// ProtoToArgsTable helper( /* provide current parsing state */ ...);
// helper.AddProtoFileDescriptor(
//     /* provide descriptor generated by tools/gen_binary_descriptors */);
// helper.InternProtoIntoArgs(const_bytes, ".perfetto.protos.MainMessage",
// row_id);
//
// Optionally one can handle particular fields as well by providing a
// ParsingOverride through AddParsingOverride.
//
// helper.AddParsingOverride("field3.field",
// [](const ProtoToArgsTable::ParsingOverrideState& state,
//    const protozero::Field& field) {
//   if (!should_handle()) {
//     return false;
//   }
//   // Parse |field| and add any rows for it to args using |state|.
//   return true;
// });
class ProtoToArgsTable {
 public:
  struct ParsingOverrideState {
    TraceProcessorContext* context;
    PacketSequenceStateGeneration* sequence_state;
  };
  using ParsingOverride = bool (*)(const ParsingOverrideState& state,
                                   const protozero::Field&,
                                   ArgsTracker::BoundInserter* inserter);

  // ScopedStringAppender will add |append| to |dest| when constructed and
  // erases the appended suffix from |dest| when it goes out of scope. Thus
  // |dest| must be valid for the entire lifetime of ScopedStringAppender.
  //
  // This is useful as we descend into a proto since the column names just
  // appended with ".field_name" as we go lower.
  //
  // I.E. message1.message2.field_name1 is a column, but we'll then need to
  // append message1.message2.field_name2 afterwards so we only need to append
  // "field_name1" within some scope. This is public so people implementing a
  // ParsingOverride can follow this same behaviour.
  class ScopedStringAppender {
   public:
    ScopedStringAppender(const std::string& append, std::string* dest);
    ~ScopedStringAppender();

   private:
    size_t old_size_;
    std::string* str_;
  };

  // |context| provides access to storage.
  explicit ProtoToArgsTable(TraceProcessorContext* context);

  // Adds a compile time reflection of a set of proto files. You must provide
  // the descriptor before attempting to parse this with
  // InternProtoIntoArgsTable().
  //
  // To generate |proto_descriptor_array| please see
  // tools/gen_binary_descriptors and ensure the proto you are interested in is
  // listed in the event_list file. You can then find your variable inside the
  // header location specified inside that python script.
  util::Status AddProtoFileDescriptor(const uint8_t* proto_descriptor_array,
                                      size_t proto_descriptor_array_size);

  // Given a view of bytes that represent a serialized protozero message of
  // |type| we will parse each field into the Args table using RowId |row|.
  //
  // Returns on any error with a status describing the problem. However any
  // added values before encountering the error will be added to the
  // args_tracker.
  //
  // Fields with ids given in |fields| are parsed using reflection, as well
  // as known (previously registered) extension fields. If |fields| is a
  // nullptr, all fields are going to be parsed.
  //
  // Note:
  // |type| must be the fully qualified name, but with a '.' added to the
  // beginning. I.E. ".perfetto.protos.TrackEvent". And must match one of the
  // descriptors already added through |AddProtoFileDescriptor|.
  //
  // IMPORTANT: currently bytes fields are not supported.
  //
  // TODO(b/145578432): Add support for byte fields.
  util::Status InternProtoFieldsIntoArgsTable(
      const protozero::ConstBytes& cb,
      const std::string& type,
      const std::vector<uint16_t>* fields,
      ArgsTracker::BoundInserter* inserter,
      PacketSequenceStateGeneration* sequence_state);

  // Installs an override for the field at the specified path. We will invoke
  // |parsing_override| when the field is encountered.
  //
  // If |parsing_override| returns false we will continue and fallback on
  // default behaviour. However if it returns true we will assume that it added
  // all required messages to the args_tracker.
  //
  // Note |field_path| must be the full path separated by periods. I.E. in the
  // proto
  //
  // message SubMessage {
  //   optional int32 field = 1;
  // }
  // message MainMessage {
  //   optional SubMessage field1 = 1;
  //   optional SubMessage field2 = 2;
  // }
  //
  // To override the handling of both SubMessage fields you must add two parsing
  // overrides. One with a |field_path| == "field1.field" and another with
  // "field2.field".
  void AddParsingOverride(std::string field_path,
                          ParsingOverride parsing_override);

 private:
  util::Status InternProtoIntoArgsTableInternal(
      const protozero::ConstBytes& cb,
      const std::string& type,
      ArgsTracker::BoundInserter* inserter,
      ParsingOverrideState state);

  util::Status InternFieldIntoArgsTable(const FieldDescriptor& field_descriptor,
                                        int repeated_field_number,
                                        ParsingOverrideState state,
                                        ArgsTracker::BoundInserter* inserter,
                                        protozero::Field field);

  using OverrideIterator =
      std::vector<std::pair<std::string, ParsingOverride>>::iterator;
  OverrideIterator FindOverride(const std::string& field);

  Variadic ConvertProtoTypeToVariadic(const FieldDescriptor& descriptor,
                                      const protozero::Field& field,
                                      ParsingOverrideState state);

  std::vector<std::pair<std::string, ParsingOverride>> overrides_;
  DescriptorPool pool_;
  std::string key_prefix_;
  std::string flat_key_prefix_;
  TraceProcessorContext* context_;
};

}  // namespace trace_processor
}  // namespace perfetto
#endif  // SRC_TRACE_PROCESSOR_IMPORTERS_PROTO_ARGS_TABLE_UTILS_H_
