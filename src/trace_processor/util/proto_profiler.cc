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

#include "src/trace_processor/util/proto_profiler.h"

#include "perfetto/protozero/packed_repeated_fields.h"
#include "perfetto/protozero/proto_decoder.h"
#include "perfetto/protozero/proto_utils.h"

namespace perfetto {
namespace trace_processor {
namespace util {

namespace {
using ::google::protobuf::FieldDescriptor;
using ::protozero::proto_utils::ProtoWireType;
}  // namespace

base::FlatHashMap<SizeProfileComputer::FieldPath,
                  SizeProfileComputer::SizeSamples,
                  SizeProfileComputer::FieldPathHasher>
SizeProfileComputer::Compute(const uint8_t* ptr,
                             size_t size,
                             const google::protobuf::Descriptor* descriptor) {
  ComputeInner(ptr, size, descriptor);
  return std::move(path_to_samples_);
}

size_t SizeProfileComputer::GetFieldSize(const protozero::Field& f) {
  uint8_t buf[10];
  switch (f.type()) {
    case protozero::proto_utils::ProtoWireType::kVarInt:
      return static_cast<size_t>(
          protozero::proto_utils::WriteVarInt(f.as_uint64(), buf) - buf);
    case protozero::proto_utils::ProtoWireType::kLengthDelimited:
      return f.size();
    case protozero::proto_utils::ProtoWireType::kFixed32:
      return 4;
    case protozero::proto_utils::ProtoWireType::kFixed64:
      return 8;
  }
  PERFETTO_FATAL("unexpected field type");  // for gcc
}

void SizeProfileComputer::ComputeInner(
    const uint8_t* ptr,
    size_t size,
    const google::protobuf::Descriptor* descriptor) {
  size_t overhead = size;
  size_t unknown = 0;
  protozero::ProtoDecoder decoder(ptr, size);

  stack_.push_back(descriptor->name());

  // Compute the size of each sub-field of this message, subtracting it
  // from overhead and possible adding it to unknown.
  for (;;) {
    if (decoder.bytes_left() == 0)
      break;
    protozero::Field field = decoder.ReadField();
    if (!field.valid()) {
      PERFETTO_ELOG("Field not valid (can mean field id >1000)");
      break;
    }

    int id = field.id();
    ProtoWireType type = field.type();
    size_t field_size = GetFieldSize(field);

    overhead -= field_size;
    const FieldDescriptor* field_descriptor = descriptor->FindFieldByNumber(id);
    if (!field_descriptor) {
      unknown += field_size;
      continue;
    }

    stack_.push_back("#" + field_descriptor->name());
    bool is_message_type =
        field_descriptor->type() == FieldDescriptor::TYPE_MESSAGE;
    if (type == ProtoWireType::kLengthDelimited && is_message_type) {
      ComputeInner(field.data(), field.size(),
                   field_descriptor->message_type());
    } else {
      stack_.push_back(field_descriptor->type_name());
      Sample(field_size);
      stack_.pop_back();
    }
    stack_.pop_back();
  }

  if (unknown) {
    stack_.push_back("#:unknown:");
    Sample(unknown);
    stack_.pop_back();
  }

  // Anything not blamed on a child is overhead for this message.
  Sample(overhead);
  stack_.pop_back();
}

void SizeProfileComputer::Sample(size_t size) {
  path_to_samples_[stack_].push_back(size);
}

}  // namespace util
}  // namespace trace_processor
}  // namespace perfetto
