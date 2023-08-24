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

#include <functional>

#include "perfetto/base/status.h"
#include "perfetto/protozero/field.h"
#include "protos/perfetto/trace/interned_data/interned_data.pbzero.h"
#include "src/trace_processor/util/descriptors.h"

namespace perfetto {
namespace trace_processor {

// TODO(altimin): Move InternedMessageView into trace_processor/util.
class InternedMessageView;
class PacketSequenceStateGeneration;

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
// ProtoToArgsParser parser(pool);
// pool.AddProtoFileDescriptor(
//     /* provide descriptor generated by tools/gen_binary_descriptors */);
// parser.ParseMessage(const_bytes, ".perfetto.protos.MainMessage",
//     /* fields */, /* delegate */);
class ProtoToArgsParser {
 public:
  explicit ProtoToArgsParser(const DescriptorPool& descriptor_pool);

  struct Key {
    Key(const std::string& flat_key, const std::string& key);
    Key(const std::string& key);
    Key();
    ~Key();

    std::string flat_key;
    std::string key;
  };

  class Delegate {
   public:
    virtual ~Delegate();

    virtual void AddInteger(const Key& key, int64_t value) = 0;
    virtual void AddUnsignedInteger(const Key& key, uint64_t value) = 0;
    virtual void AddString(const Key& key,
                           const protozero::ConstChars& value) = 0;
    virtual void AddString(const Key& key, const std::string& value) = 0;
    virtual void AddDouble(const Key& key, double value) = 0;
    virtual void AddPointer(const Key& key, const void* value) = 0;
    virtual void AddBoolean(const Key& key, bool value) = 0;
    virtual void AddBytes(const Key& key, const protozero::ConstBytes& value) {
      // In the absence of a better implementation default to showing
      // bytes as string with the size:
      std::string msg = "<bytes size=" + std::to_string(value.size) + ">";
      AddString(key, msg);
    }
    // Returns whether an entry was added or not.
    virtual bool AddJson(const Key& key,
                         const protozero::ConstChars& value) = 0;
    virtual void AddNull(const Key& key) = 0;

    virtual size_t GetArrayEntryIndex(const std::string& array_key) = 0;
    virtual size_t IncrementArrayEntryIndex(const std::string& array_key) = 0;

    virtual PacketSequenceStateGeneration* seq_state() = 0;

    virtual int64_t packet_timestamp() { return 0; }

    template <typename FieldMetadata>
    typename FieldMetadata::cpp_field_type::Decoder* GetInternedMessage(
        FieldMetadata,
        uint64_t iid) {
      static_assert(std::is_base_of<protozero::proto_utils::FieldMetadataBase,
                                    FieldMetadata>::value,
                    "Field metadata should be a subclass of FieldMetadataBase");
      static_assert(std::is_same<typename FieldMetadata::message_type,
                                 protos::pbzero::InternedData>::value,
                    "Field should belong to InternedData proto");
      auto* interned_message_view =
          GetInternedMessageView(FieldMetadata::kFieldId, iid);
      if (!interned_message_view)
        return nullptr;
      return interned_message_view->template GetOrCreateDecoder<
          typename FieldMetadata::cpp_field_type>();
    }

   protected:
    virtual InternedMessageView* GetInternedMessageView(uint32_t field_id,
                                                        uint64_t iid) = 0;
  };

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
  base::Status ParseMessage(const protozero::ConstBytes& cb,
                            const std::string& type,
                            const std::vector<uint32_t>* allowed_fields,
                            Delegate& delegate,
                            int* unknown_extensions = nullptr);

  // This class is responsible for resetting the current key prefix to the old
  // value when deleted or reset.
  struct ScopedNestedKeyContext {
   public:
    ~ScopedNestedKeyContext();
    ScopedNestedKeyContext(ScopedNestedKeyContext&&);
    ScopedNestedKeyContext(const ScopedNestedKeyContext&) = delete;
    ScopedNestedKeyContext& operator=(const ScopedNestedKeyContext&) = delete;

    const Key& key() const { return key_; }

    // Clear this context, which strips the latest suffix from |key_| and sets
    // it to the state before the nested context was created.
    void RemoveFieldSuffix();

   private:
    friend class ProtoToArgsParser;

    ScopedNestedKeyContext(Key& old_value);

    struct ScopedStringAppender;

    Key& key_;
    std::optional<size_t> old_flat_key_length_ = std::nullopt;
    std::optional<size_t> old_key_length_ = std::nullopt;
  };

  // These methods can be called from parsing overrides to enter nested
  // contexts. The contexts are left when the returned scope is destroyed or
  // RemoveFieldSuffix() is called.
  ScopedNestedKeyContext EnterDictionary(const std::string& key);
  ScopedNestedKeyContext EnterArray(size_t index);

  using ParsingOverrideForField =
      std::function<std::optional<base::Status>(const protozero::Field&,
                                                Delegate& delegate)>;

  // Installs an override for the field at the specified path. We will invoke
  // |parsing_override| when the field is encountered.
  //
  // The return value of |parsing_override| indicates whether the override
  // parsed the sub-message and ProtoToArgsParser should skip it (std::nullopt)
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
  void AddParsingOverrideForField(const std::string& field_path,
                                  ParsingOverrideForField parsing_override);

  using ParsingOverrideForType = std::function<std::optional<base::Status>(
      ScopedNestedKeyContext& key,
      const protozero::ConstBytes& data,
      Delegate& delegate)>;

  // Installs an override for all fields with the given type. We will invoke
  // |parsing_override| when a field with the given message type is encountered.
  // Note that the path-based overrides take precedence over type overrides.
  //
  // The return value of |parsing_override| indicates whether the override
  // parsed the sub-message and ProtoToArgsParser should skip it (std::nullopt)
  // or the sub-message should continue to be parsed by ProtoToArgsParser using
  // the descriptor (base::Status).
  //
  //
  // For example, given the following protos and a type override for SubMessage,
  // all three fields will be parsed using this override.
  //
  // message SubMessage {
  //   optional int32 value = 1;
  // }
  //
  // message MainMessage1 {
  //   optional SubMessage field1 = 1;
  //   optional SubMessage field2 = 2;
  // }
  //
  // message MainMessage2 {
  //   optional SubMessage field3 = 1;
  // }
  void AddParsingOverrideForType(const std::string& message_type,
                                 ParsingOverrideForType parsing_override);

 private:
  base::Status ParseField(const FieldDescriptor& field_descriptor,
                          int repeated_field_number,
                          protozero::Field field,
                          Delegate& delegate,
                          int* unknown_extensions);

  base::Status ParsePackedField(
      const FieldDescriptor& field_descriptor,
      std::unordered_map<size_t, int>& repeated_field_index,
      protozero::Field field,
      Delegate& delegate,
      int* unknown_extensions);

  std::optional<base::Status> MaybeApplyOverrideForField(
      const protozero::Field&,
      Delegate& delegate);

  std::optional<base::Status> MaybeApplyOverrideForType(
      const std::string& message_type,
      ScopedNestedKeyContext& key,
      const protozero::ConstBytes& data,
      Delegate& delegate);

  // A type override can call |key.RemoveFieldSuffix()| if it wants to exclude
  // the overriden field's name from the parsed args' keys.
  base::Status ParseMessageInternal(ScopedNestedKeyContext& key,
                                    const protozero::ConstBytes& cb,
                                    const std::string& type,
                                    const std::vector<uint32_t>* fields,
                                    Delegate& delegate,
                                    int* unknown_extensions);

  base::Status ParseSimpleField(const FieldDescriptor& desciptor,
                                const protozero::Field& field,
                                Delegate& delegate);

  std::unordered_map<std::string, ParsingOverrideForField> field_overrides_;
  std::unordered_map<std::string, ParsingOverrideForType> type_overrides_;
  const DescriptorPool& pool_;
  Key key_prefix_;
};

}  // namespace util
}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_UTIL_PROTO_TO_ARGS_PARSER_H_
