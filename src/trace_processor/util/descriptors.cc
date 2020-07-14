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

#include "src/trace_processor/util/descriptors.h"
#include "perfetto/ext/base/string_view.h"
#include "perfetto/protozero/field.h"

#include "protos/perfetto/common/descriptor.pbzero.h"

namespace perfetto {
namespace trace_processor {

FieldDescriptor CreateFieldFromDecoder(
    const protos::pbzero::FieldDescriptorProto::Decoder& f_decoder,
    bool is_extension) {
  using FieldDescriptorProto = protos::pbzero::FieldDescriptorProto;
  std::string type_name =
      f_decoder.has_type_name()
          ? base::StringView(f_decoder.type_name()).ToStdString()
          : "";
  // TODO(lalitm): add support for enums here.
  uint32_t type =
      f_decoder.has_type()
          ? static_cast<uint32_t>(f_decoder.type())
          : static_cast<uint32_t>(FieldDescriptorProto::TYPE_MESSAGE);
  return FieldDescriptor(
      base::StringView(f_decoder.name()).ToStdString(),
      static_cast<uint32_t>(f_decoder.number()), type, std::move(type_name),
      f_decoder.label() == FieldDescriptorProto::LABEL_REPEATED, is_extension);
}

base::Optional<uint32_t> DescriptorPool::ResolveShortType(
    const std::string& parent_path,
    const std::string& short_type) {
  PERFETTO_DCHECK(!short_type.empty());

  std::string search_path = short_type[0] == '.'
                                ? parent_path + short_type
                                : parent_path + '.' + short_type;
  auto opt_idx = FindDescriptorIdx(search_path);
  if (opt_idx)
    return opt_idx;

  if (parent_path.empty())
    return base::nullopt;

  auto parent_dot_idx = parent_path.rfind('.');
  auto parent_substr = parent_dot_idx == std::string::npos
                           ? ""
                           : parent_path.substr(0, parent_dot_idx);
  return ResolveShortType(parent_substr, short_type);
}

util::Status DescriptorPool::AddExtensionField(
    const std::string& package_name,
    protozero::ConstBytes field_desc_proto) {
  using FieldDescriptorProto = protos::pbzero::FieldDescriptorProto;
  FieldDescriptorProto::Decoder f_decoder(field_desc_proto);
  auto field = CreateFieldFromDecoder(f_decoder, true);

  auto extendee_name = base::StringView(f_decoder.extendee()).ToStdString();
  PERFETTO_CHECK(!extendee_name.empty());
  if (extendee_name[0] != '.') {
    // Only prepend if the extendee is not fully qualified
    extendee_name = package_name + "." + extendee_name;
  }
  auto extendee = FindDescriptorIdx(extendee_name);
  if (!extendee.has_value()) {
    return util::ErrStatus("Extendee does not exist %s", extendee_name.c_str());
  }
  descriptors_[extendee.value()].AddField(field);
  return util::OkStatus();
}

void DescriptorPool::AddNestedProtoDescriptors(
    const std::string& package_name,
    base::Optional<uint32_t> parent_idx,
    protozero::ConstBytes descriptor_proto,
    std::vector<ExtensionInfo>* extensions) {
  protos::pbzero::DescriptorProto::Decoder decoder(descriptor_proto);

  auto parent_name =
      parent_idx ? descriptors_[*parent_idx].full_name() : package_name;
  auto full_name =
      parent_name + "." + base::StringView(decoder.name()).ToStdString();

  using FieldDescriptorProto = protos::pbzero::FieldDescriptorProto;
  ProtoDescriptor proto_descriptor(package_name, full_name,
                                   ProtoDescriptor::Type::kMessage, parent_idx);
  for (auto it = decoder.field(); it; ++it) {
    FieldDescriptorProto::Decoder f_decoder(*it);
    proto_descriptor.AddField(CreateFieldFromDecoder(f_decoder, false));
  }
  descriptors_.emplace_back(std::move(proto_descriptor));

  auto idx = static_cast<uint32_t>(descriptors_.size()) - 1;
  for (auto it = decoder.enum_type(); it; ++it) {
    AddEnumProtoDescriptors(package_name, idx, *it);
  }
  for (auto it = decoder.nested_type(); it; ++it) {
    AddNestedProtoDescriptors(package_name, idx, *it, extensions);
  }
  for (auto ext_it = decoder.extension(); ext_it; ++ext_it) {
    extensions->emplace_back(package_name, *ext_it);
  }
}

void DescriptorPool::AddEnumProtoDescriptors(
    const std::string& package_name,
    base::Optional<uint32_t> parent_idx,
    protozero::ConstBytes descriptor_proto) {
  protos::pbzero::EnumDescriptorProto::Decoder decoder(descriptor_proto);

  auto parent_name =
      parent_idx ? descriptors_[*parent_idx].full_name() : package_name;
  auto full_name =
      parent_name + "." + base::StringView(decoder.name()).ToStdString();

  ProtoDescriptor proto_descriptor(package_name, full_name,
                                   ProtoDescriptor::Type::kEnum, base::nullopt);
  for (auto it = decoder.value(); it; ++it) {
    protos::pbzero::EnumValueDescriptorProto::Decoder enum_value(it->data(),
                                                                 it->size());
    proto_descriptor.AddEnumValue(enum_value.number(),
                                  enum_value.name().ToStdString());
  }
  descriptors_.emplace_back(std::move(proto_descriptor));
}

util::Status DescriptorPool::AddFromFileDescriptorSet(
    const uint8_t* file_descriptor_set_proto,
    size_t size) {
  // First pass: extract all the message descriptors from the file and add them
  // to the pool.
  protos::pbzero::FileDescriptorSet::Decoder proto(file_descriptor_set_proto,
                                                   size);
  std::vector<ExtensionInfo> extensions;
  for (auto it = proto.file(); it; ++it) {
    protos::pbzero::FileDescriptorProto::Decoder file(*it);
    std::string package = "." + base::StringView(file.package()).ToStdString();
    for (auto message_it = file.message_type(); message_it; ++message_it) {
      AddNestedProtoDescriptors(package, base::nullopt, *message_it,
                                &extensions);
    }
    for (auto enum_it = file.enum_type(); enum_it; ++enum_it) {
      AddEnumProtoDescriptors(package, base::nullopt, *enum_it);
    }
    for (auto ext_it = file.extension(); ext_it; ++ext_it) {
      extensions.emplace_back(package, *ext_it);
    }
  }

  // Second pass: extract all the extension protos and add them to the real
  // protos.
  for (auto it = proto.file(); it; ++it) {
    for (auto extension : extensions) {
      auto status = AddExtensionField(extension.first, extension.second);
      if (!status.ok())
        return status;
    }
  }

  // Third pass: resolve the types of all the fields to the correct indiices.
  using FieldDescriptorProto = protos::pbzero::FieldDescriptorProto;
  for (auto& descriptor : descriptors_) {
    for (auto& field : *descriptor.mutable_fields()) {
      if (!field.resolved_type_name().empty())
        continue;

      if (field.type() == FieldDescriptorProto::TYPE_MESSAGE ||
          field.type() == FieldDescriptorProto::TYPE_ENUM) {
        auto opt_desc =
            ResolveShortType(descriptor.full_name(), field.raw_type_name());
        if (!opt_desc.has_value()) {
          return util::ErrStatus(
              "Unable to find short type %s in field inside message %s",
              field.raw_type_name().c_str(), descriptor.full_name().c_str());
        }
        field.set_resolved_type_name(
            descriptors_[opt_desc.value()].full_name());
      }
    }
  }
  return util::OkStatus();
}

base::Optional<uint32_t> DescriptorPool::FindDescriptorIdx(
    const std::string& full_name) const {
  auto it = std::find_if(descriptors_.begin(), descriptors_.end(),
                         [full_name](const ProtoDescriptor& desc) {
                           return desc.full_name() == full_name;
                         });
  auto idx = static_cast<uint32_t>(std::distance(descriptors_.begin(), it));
  return idx < descriptors_.size() ? base::Optional<uint32_t>(idx)
                                   : base::nullopt;
}

ProtoDescriptor::ProtoDescriptor(std::string package_name,
                                 std::string full_name,
                                 Type type,
                                 base::Optional<uint32_t> parent_id)
    : package_name_(std::move(package_name)),
      full_name_(std::move(full_name)),
      type_(type),
      parent_id_(parent_id) {}

FieldDescriptor::FieldDescriptor(std::string name,
                                 uint32_t number,
                                 uint32_t type,
                                 std::string raw_type_name,
                                 bool is_repeated,
                                 bool is_extension)
    : name_(std::move(name)),
      number_(number),
      type_(type),
      raw_type_name_(std::move(raw_type_name)),
      is_repeated_(is_repeated),
      is_extension_(is_extension) {}

}  // namespace trace_processor
}  // namespace perfetto
