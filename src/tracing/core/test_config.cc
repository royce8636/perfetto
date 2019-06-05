/*
 * Copyright (C) 2017 The Android Open Source Project
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

/*******************************************************************************
 * AUTOGENERATED - DO NOT EDIT
 *******************************************************************************
 * This file has been generated from the protobuf message
 * perfetto/config/test_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#include "perfetto/tracing/core/test_config.h"

#include "perfetto/config/test_config.pb.h"

namespace perfetto {

TestConfig::TestConfig() = default;
TestConfig::~TestConfig() = default;
TestConfig::TestConfig(const TestConfig&) = default;
TestConfig& TestConfig::operator=(const TestConfig&) = default;
TestConfig::TestConfig(TestConfig&&) noexcept = default;
TestConfig& TestConfig::operator=(TestConfig&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool TestConfig::operator==(const TestConfig& other) const {
  return (message_count_ == other.message_count_) &&
         (max_messages_per_second_ == other.max_messages_per_second_) &&
         (seed_ == other.seed_) && (message_size_ == other.message_size_) &&
         (send_batch_on_register_ == other.send_batch_on_register_) &&
         (dummy_fields_ == other.dummy_fields_);
}
#pragma GCC diagnostic pop

void TestConfig::ParseRawProto(const std::string& raw) {
  perfetto::protos::TestConfig proto;
  proto.ParseFromString(raw);
  FromProto(proto);
}

void TestConfig::FromProto(const perfetto::protos::TestConfig& proto) {
  static_assert(sizeof(message_count_) == sizeof(proto.message_count()),
                "size mismatch");
  message_count_ = static_cast<decltype(message_count_)>(proto.message_count());

  static_assert(sizeof(max_messages_per_second_) ==
                    sizeof(proto.max_messages_per_second()),
                "size mismatch");
  max_messages_per_second_ = static_cast<decltype(max_messages_per_second_)>(
      proto.max_messages_per_second());

  static_assert(sizeof(seed_) == sizeof(proto.seed()), "size mismatch");
  seed_ = static_cast<decltype(seed_)>(proto.seed());

  static_assert(sizeof(message_size_) == sizeof(proto.message_size()),
                "size mismatch");
  message_size_ = static_cast<decltype(message_size_)>(proto.message_size());

  static_assert(
      sizeof(send_batch_on_register_) == sizeof(proto.send_batch_on_register()),
      "size mismatch");
  send_batch_on_register_ = static_cast<decltype(send_batch_on_register_)>(
      proto.send_batch_on_register());

  dummy_fields_.FromProto(proto.dummy_fields());
  unknown_fields_ = proto.unknown_fields();
}

void TestConfig::ToProto(perfetto::protos::TestConfig* proto) const {
  proto->Clear();

  static_assert(sizeof(message_count_) == sizeof(proto->message_count()),
                "size mismatch");
  proto->set_message_count(
      static_cast<decltype(proto->message_count())>(message_count_));

  static_assert(sizeof(max_messages_per_second_) ==
                    sizeof(proto->max_messages_per_second()),
                "size mismatch");
  proto->set_max_messages_per_second(
      static_cast<decltype(proto->max_messages_per_second())>(
          max_messages_per_second_));

  static_assert(sizeof(seed_) == sizeof(proto->seed()), "size mismatch");
  proto->set_seed(static_cast<decltype(proto->seed())>(seed_));

  static_assert(sizeof(message_size_) == sizeof(proto->message_size()),
                "size mismatch");
  proto->set_message_size(
      static_cast<decltype(proto->message_size())>(message_size_));

  static_assert(sizeof(send_batch_on_register_) ==
                    sizeof(proto->send_batch_on_register()),
                "size mismatch");
  proto->set_send_batch_on_register(
      static_cast<decltype(proto->send_batch_on_register())>(
          send_batch_on_register_));

  dummy_fields_.ToProto(proto->mutable_dummy_fields());
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

TestConfig::DummyFields::DummyFields() = default;
TestConfig::DummyFields::~DummyFields() = default;
TestConfig::DummyFields::DummyFields(const TestConfig::DummyFields&) = default;
TestConfig::DummyFields& TestConfig::DummyFields::operator=(
    const TestConfig::DummyFields&) = default;
TestConfig::DummyFields::DummyFields(TestConfig::DummyFields&&) noexcept =
    default;
TestConfig::DummyFields& TestConfig::DummyFields::operator=(
    TestConfig::DummyFields&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool TestConfig::DummyFields::operator==(
    const TestConfig::DummyFields& other) const {
  return (field_uint32_ == other.field_uint32_) &&
         (field_int32_ == other.field_int32_) &&
         (field_uint64_ == other.field_uint64_) &&
         (field_int64_ == other.field_int64_) &&
         (field_fixed64_ == other.field_fixed64_) &&
         (field_sfixed64_ == other.field_sfixed64_) &&
         (field_fixed32_ == other.field_fixed32_) &&
         (field_sfixed32_ == other.field_sfixed32_) &&
         (field_double_ == other.field_double_) &&
         (field_float_ == other.field_float_) &&
         (field_sint64_ == other.field_sint64_) &&
         (field_sint32_ == other.field_sint32_) &&
         (field_string_ == other.field_string_) &&
         (field_bytes_ == other.field_bytes_);
}
#pragma GCC diagnostic pop

void TestConfig::DummyFields::ParseRawProto(const std::string& raw) {
  perfetto::protos::TestConfig_DummyFields proto;
  proto.ParseFromString(raw);
  FromProto(proto);
}

void TestConfig::DummyFields::FromProto(
    const perfetto::protos::TestConfig_DummyFields& proto) {
  static_assert(sizeof(field_uint32_) == sizeof(proto.field_uint32()),
                "size mismatch");
  field_uint32_ = static_cast<decltype(field_uint32_)>(proto.field_uint32());

  static_assert(sizeof(field_int32_) == sizeof(proto.field_int32()),
                "size mismatch");
  field_int32_ = static_cast<decltype(field_int32_)>(proto.field_int32());

  static_assert(sizeof(field_uint64_) == sizeof(proto.field_uint64()),
                "size mismatch");
  field_uint64_ = static_cast<decltype(field_uint64_)>(proto.field_uint64());

  static_assert(sizeof(field_int64_) == sizeof(proto.field_int64()),
                "size mismatch");
  field_int64_ = static_cast<decltype(field_int64_)>(proto.field_int64());

  static_assert(sizeof(field_fixed64_) == sizeof(proto.field_fixed64()),
                "size mismatch");
  field_fixed64_ = static_cast<decltype(field_fixed64_)>(proto.field_fixed64());

  static_assert(sizeof(field_sfixed64_) == sizeof(proto.field_sfixed64()),
                "size mismatch");
  field_sfixed64_ =
      static_cast<decltype(field_sfixed64_)>(proto.field_sfixed64());

  static_assert(sizeof(field_fixed32_) == sizeof(proto.field_fixed32()),
                "size mismatch");
  field_fixed32_ = static_cast<decltype(field_fixed32_)>(proto.field_fixed32());

  static_assert(sizeof(field_sfixed32_) == sizeof(proto.field_sfixed32()),
                "size mismatch");
  field_sfixed32_ =
      static_cast<decltype(field_sfixed32_)>(proto.field_sfixed32());

  static_assert(sizeof(field_double_) == sizeof(proto.field_double()),
                "size mismatch");
  field_double_ = static_cast<decltype(field_double_)>(proto.field_double());

  static_assert(sizeof(field_float_) == sizeof(proto.field_float()),
                "size mismatch");
  field_float_ = static_cast<decltype(field_float_)>(proto.field_float());

  static_assert(sizeof(field_sint64_) == sizeof(proto.field_sint64()),
                "size mismatch");
  field_sint64_ = static_cast<decltype(field_sint64_)>(proto.field_sint64());

  static_assert(sizeof(field_sint32_) == sizeof(proto.field_sint32()),
                "size mismatch");
  field_sint32_ = static_cast<decltype(field_sint32_)>(proto.field_sint32());

  static_assert(sizeof(field_string_) == sizeof(proto.field_string()),
                "size mismatch");
  field_string_ = static_cast<decltype(field_string_)>(proto.field_string());

  static_assert(sizeof(field_bytes_) == sizeof(proto.field_bytes()),
                "size mismatch");
  field_bytes_ = static_cast<decltype(field_bytes_)>(proto.field_bytes());
  unknown_fields_ = proto.unknown_fields();
}

void TestConfig::DummyFields::ToProto(
    perfetto::protos::TestConfig_DummyFields* proto) const {
  proto->Clear();

  static_assert(sizeof(field_uint32_) == sizeof(proto->field_uint32()),
                "size mismatch");
  proto->set_field_uint32(
      static_cast<decltype(proto->field_uint32())>(field_uint32_));

  static_assert(sizeof(field_int32_) == sizeof(proto->field_int32()),
                "size mismatch");
  proto->set_field_int32(
      static_cast<decltype(proto->field_int32())>(field_int32_));

  static_assert(sizeof(field_uint64_) == sizeof(proto->field_uint64()),
                "size mismatch");
  proto->set_field_uint64(
      static_cast<decltype(proto->field_uint64())>(field_uint64_));

  static_assert(sizeof(field_int64_) == sizeof(proto->field_int64()),
                "size mismatch");
  proto->set_field_int64(
      static_cast<decltype(proto->field_int64())>(field_int64_));

  static_assert(sizeof(field_fixed64_) == sizeof(proto->field_fixed64()),
                "size mismatch");
  proto->set_field_fixed64(
      static_cast<decltype(proto->field_fixed64())>(field_fixed64_));

  static_assert(sizeof(field_sfixed64_) == sizeof(proto->field_sfixed64()),
                "size mismatch");
  proto->set_field_sfixed64(
      static_cast<decltype(proto->field_sfixed64())>(field_sfixed64_));

  static_assert(sizeof(field_fixed32_) == sizeof(proto->field_fixed32()),
                "size mismatch");
  proto->set_field_fixed32(
      static_cast<decltype(proto->field_fixed32())>(field_fixed32_));

  static_assert(sizeof(field_sfixed32_) == sizeof(proto->field_sfixed32()),
                "size mismatch");
  proto->set_field_sfixed32(
      static_cast<decltype(proto->field_sfixed32())>(field_sfixed32_));

  static_assert(sizeof(field_double_) == sizeof(proto->field_double()),
                "size mismatch");
  proto->set_field_double(
      static_cast<decltype(proto->field_double())>(field_double_));

  static_assert(sizeof(field_float_) == sizeof(proto->field_float()),
                "size mismatch");
  proto->set_field_float(
      static_cast<decltype(proto->field_float())>(field_float_));

  static_assert(sizeof(field_sint64_) == sizeof(proto->field_sint64()),
                "size mismatch");
  proto->set_field_sint64(
      static_cast<decltype(proto->field_sint64())>(field_sint64_));

  static_assert(sizeof(field_sint32_) == sizeof(proto->field_sint32()),
                "size mismatch");
  proto->set_field_sint32(
      static_cast<decltype(proto->field_sint32())>(field_sint32_));

  static_assert(sizeof(field_string_) == sizeof(proto->field_string()),
                "size mismatch");
  proto->set_field_string(
      static_cast<decltype(proto->field_string())>(field_string_));

  static_assert(sizeof(field_bytes_) == sizeof(proto->field_bytes()),
                "size mismatch");
  proto->set_field_bytes(
      static_cast<decltype(proto->field_bytes())>(field_bytes_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

}  // namespace perfetto
