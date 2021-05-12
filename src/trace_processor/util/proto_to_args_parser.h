/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef SRC_TRACE_PROCESSOR_UTIL_PROTO_TO_ARGS_PARSER_H_
#define SRC_TRACE_PROCESSOR_UTIL_PROTO_TO_ARGS_PARSER_H_

#include "perfetto/base/status.h"
#include "perfetto/protozero/field.h"
#include "src/trace_processor/util/descriptors.h"

namespace perfetto {
namespace trace_processor {
namespace util {

// ProtoToArgsParser encapsulates the process of taking an arbitrary proto and
// parsing it into key-value arg pairs. This is done by traversing
// the proto using reflection (with descriptors from |descriptor_pool|)
// and passing the parsed data to |Delegate| callbacks.
//
// E.g. given a proto like
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
// DescriptorPool pool;
// ProtoToArgsParser parser(&pool);
// pool.AddProtoFileDescriptor(
//     /* provide descriptor generated by tools/gen_binary_descriptors */);
// parser.ParseMessage(const_bytes, ".perfetto.protos.MainMessage",
//     /* fields */, /* delegate */);
class ProtoToArgsParser {
 public:
  explicit ProtoToArgsParser(const DescriptorPool& descriptor_pool);

  struct Key {
    std::string key;
    std::string flat_key;
  };

  class Delegate {
   public:
    virtual ~Delegate();

    virtual void AddInteger(const Key& key, int64_t value) = 0;
    virtual void AddUnsignedInteger(const Key& key, uint64_t value) = 0;
    virtual void AddString(const Key& key,
                           const protozero::ConstChars& value) = 0;
    virtual void AddDouble(const Key& key, double value) = 0;
    virtual void AddPointer(const Key& key, const void* value) = 0;
    virtual void AddBoolean(const Key& key, bool value) = 0;
    virtual void AddJson(const Key& key,
                         const protozero::ConstChars& value) = 0;
  };

  using ParsingOverride =
      std::function<base::Optional<base::Status>(const protozero::Field&,
                                                 Delegate& delegate)>;

  // Installs an override for the field at the specified path. We will invoke
  // |parsing_override| when the field is encountered.
  //
  // The return value of |parsing_override| indicates whether the override
  // parsed the sub-message and ProtoToArgsParser should skip it (base::nullopt)
  // or the sub-message should continue to be parsed by ProtoToArgsParser using
  // the descriptor (base::Status).
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

  // Given a view of bytes that represent a serialized protozero message of
  // |type| we will parse each field.
  //
  // Returns on any error with a status describing the problem. However any
  // added values before encountering the error will be parsed and forwarded to
  // the delegate.
  //
  // Fields with ids given in |fields| are parsed using reflection, as well
  // as known (previously registered) extension fields. If |allowed_fields| is a
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
  base::Status ParseMessage(const protozero::ConstBytes& cb,
                            const std::string& type,
                            const std::vector<uint16_t>* allowed_fields,
                            Delegate& delegate);

 private:
  base::Status ParseField(const FieldDescriptor& field_descriptor,
                          int repeated_field_number,
                          protozero::Field field,
                          Delegate& delegate);

  base::Optional<base::Status> MaybeApplyOverride(const protozero::Field&,
                                                  Delegate& delegate);

  base::Status ParseSimpleField(const FieldDescriptor& desciptor,
                                const protozero::Field& field,
                                Delegate& delegate);

  std::unordered_map<std::string, ParsingOverride> overrides_;
  const DescriptorPool& pool_;
  Key key_prefix_;
};

}  // namespace util
}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_UTIL_PROTO_TO_ARGS_PARSER_H_
