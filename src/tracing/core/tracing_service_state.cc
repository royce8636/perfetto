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
 * perfetto/common/tracing_service_state.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#include "perfetto/tracing/core/tracing_service_state.h"

#include "perfetto/common/data_source_descriptor.pb.h"
#include "perfetto/common/tracing_service_state.pb.h"

namespace perfetto {

TracingServiceState::TracingServiceState() = default;
TracingServiceState::~TracingServiceState() = default;
TracingServiceState::TracingServiceState(const TracingServiceState&) = default;
TracingServiceState& TracingServiceState::operator=(
    const TracingServiceState&) = default;
TracingServiceState::TracingServiceState(TracingServiceState&&) noexcept =
    default;
TracingServiceState& TracingServiceState::operator=(TracingServiceState&&) =
    default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool TracingServiceState::operator==(const TracingServiceState& other) const {
  return (producers_ == other.producers_) &&
         (data_sources_ == other.data_sources_) &&
         (num_sessions_ == other.num_sessions_) &&
         (num_sessions_started_ == other.num_sessions_started_);
}
#pragma GCC diagnostic pop

void TracingServiceState::ParseRawProto(const std::string& raw) {
  perfetto::protos::TracingServiceState proto;
  proto.ParseFromString(raw);
  FromProto(proto);
}

void TracingServiceState::FromProto(
    const perfetto::protos::TracingServiceState& proto) {
  producers_.clear();
  for (const auto& field : proto.producers()) {
    producers_.emplace_back();
    producers_.back().FromProto(field);
  }

  data_sources_.clear();
  for (const auto& field : proto.data_sources()) {
    data_sources_.emplace_back();
    data_sources_.back().FromProto(field);
  }

  static_assert(sizeof(num_sessions_) == sizeof(proto.num_sessions()),
                "size mismatch");
  num_sessions_ = static_cast<decltype(num_sessions_)>(proto.num_sessions());

  static_assert(
      sizeof(num_sessions_started_) == sizeof(proto.num_sessions_started()),
      "size mismatch");
  num_sessions_started_ = static_cast<decltype(num_sessions_started_)>(
      proto.num_sessions_started());
  unknown_fields_ = proto.unknown_fields();
}

void TracingServiceState::ToProto(
    perfetto::protos::TracingServiceState* proto) const {
  proto->Clear();

  for (const auto& it : producers_) {
    auto* entry = proto->add_producers();
    it.ToProto(entry);
  }

  for (const auto& it : data_sources_) {
    auto* entry = proto->add_data_sources();
    it.ToProto(entry);
  }

  static_assert(sizeof(num_sessions_) == sizeof(proto->num_sessions()),
                "size mismatch");
  proto->set_num_sessions(
      static_cast<decltype(proto->num_sessions())>(num_sessions_));

  static_assert(
      sizeof(num_sessions_started_) == sizeof(proto->num_sessions_started()),
      "size mismatch");
  proto->set_num_sessions_started(
      static_cast<decltype(proto->num_sessions_started())>(
          num_sessions_started_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

TracingServiceState::Producer::Producer() = default;
TracingServiceState::Producer::~Producer() = default;
TracingServiceState::Producer::Producer(const TracingServiceState::Producer&) =
    default;
TracingServiceState::Producer& TracingServiceState::Producer::operator=(
    const TracingServiceState::Producer&) = default;
TracingServiceState::Producer::Producer(
    TracingServiceState::Producer&&) noexcept = default;
TracingServiceState::Producer& TracingServiceState::Producer::operator=(
    TracingServiceState::Producer&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool TracingServiceState::Producer::operator==(
    const TracingServiceState::Producer& other) const {
  return (id_ == other.id_) && (name_ == other.name_) && (uid_ == other.uid_);
}
#pragma GCC diagnostic pop

void TracingServiceState::Producer::ParseRawProto(const std::string& raw) {
  perfetto::protos::TracingServiceState_Producer proto;
  proto.ParseFromString(raw);
  FromProto(proto);
}

void TracingServiceState::Producer::FromProto(
    const perfetto::protos::TracingServiceState_Producer& proto) {
  static_assert(sizeof(id_) == sizeof(proto.id()), "size mismatch");
  id_ = static_cast<decltype(id_)>(proto.id());

  static_assert(sizeof(name_) == sizeof(proto.name()), "size mismatch");
  name_ = static_cast<decltype(name_)>(proto.name());

  static_assert(sizeof(uid_) == sizeof(proto.uid()), "size mismatch");
  uid_ = static_cast<decltype(uid_)>(proto.uid());
  unknown_fields_ = proto.unknown_fields();
}

void TracingServiceState::Producer::ToProto(
    perfetto::protos::TracingServiceState_Producer* proto) const {
  proto->Clear();

  static_assert(sizeof(id_) == sizeof(proto->id()), "size mismatch");
  proto->set_id(static_cast<decltype(proto->id())>(id_));

  static_assert(sizeof(name_) == sizeof(proto->name()), "size mismatch");
  proto->set_name(static_cast<decltype(proto->name())>(name_));

  static_assert(sizeof(uid_) == sizeof(proto->uid()), "size mismatch");
  proto->set_uid(static_cast<decltype(proto->uid())>(uid_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

TracingServiceState::DataSource::DataSource() = default;
TracingServiceState::DataSource::~DataSource() = default;
TracingServiceState::DataSource::DataSource(
    const TracingServiceState::DataSource&) = default;
TracingServiceState::DataSource& TracingServiceState::DataSource::operator=(
    const TracingServiceState::DataSource&) = default;
TracingServiceState::DataSource::DataSource(
    TracingServiceState::DataSource&&) noexcept = default;
TracingServiceState::DataSource& TracingServiceState::DataSource::operator=(
    TracingServiceState::DataSource&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool TracingServiceState::DataSource::operator==(
    const TracingServiceState::DataSource& other) const {
  return (descriptor_ == other.descriptor_) &&
         (producer_id_ == other.producer_id_);
}
#pragma GCC diagnostic pop

void TracingServiceState::DataSource::ParseRawProto(const std::string& raw) {
  perfetto::protos::TracingServiceState_DataSource proto;
  proto.ParseFromString(raw);
  FromProto(proto);
}

void TracingServiceState::DataSource::FromProto(
    const perfetto::protos::TracingServiceState_DataSource& proto) {
  descriptor_.FromProto(proto.descriptor());

  static_assert(sizeof(producer_id_) == sizeof(proto.producer_id()),
                "size mismatch");
  producer_id_ = static_cast<decltype(producer_id_)>(proto.producer_id());
  unknown_fields_ = proto.unknown_fields();
}

void TracingServiceState::DataSource::ToProto(
    perfetto::protos::TracingServiceState_DataSource* proto) const {
  proto->Clear();

  descriptor_.ToProto(proto->mutable_descriptor());

  static_assert(sizeof(producer_id_) == sizeof(proto->producer_id()),
                "size mismatch");
  proto->set_producer_id(
      static_cast<decltype(proto->producer_id())>(producer_id_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

}  // namespace perfetto
