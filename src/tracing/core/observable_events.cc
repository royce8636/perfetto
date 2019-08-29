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
 * protos/perfetto/common/observable_events.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#include "perfetto/ext/tracing/core/observable_events.h"

#include "protos/perfetto/common/observable_events.pb.h"

namespace perfetto {

ObservableEvents::ObservableEvents() = default;
ObservableEvents::~ObservableEvents() = default;
ObservableEvents::ObservableEvents(const ObservableEvents&) = default;
ObservableEvents& ObservableEvents::operator=(const ObservableEvents&) =
    default;
ObservableEvents::ObservableEvents(ObservableEvents&&) noexcept = default;
ObservableEvents& ObservableEvents::operator=(ObservableEvents&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool ObservableEvents::operator==(const ObservableEvents& other) const {
  return (instance_state_changes_ == other.instance_state_changes_);
}
#pragma GCC diagnostic pop

void ObservableEvents::ParseRawProto(const std::string& raw) {
  perfetto::protos::ObservableEvents proto;
  proto.ParseFromString(raw);
  FromProto(proto);
}

void ObservableEvents::FromProto(
    const perfetto::protos::ObservableEvents& proto) {
  instance_state_changes_.clear();
  for (const auto& field : proto.instance_state_changes()) {
    instance_state_changes_.emplace_back();
    instance_state_changes_.back().FromProto(field);
  }
  unknown_fields_ = proto.unknown_fields();
}

void ObservableEvents::ToProto(
    perfetto::protos::ObservableEvents* proto) const {
  proto->Clear();

  for (const auto& it : instance_state_changes_) {
    auto* entry = proto->add_instance_state_changes();
    it.ToProto(entry);
  }
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

ObservableEvents::DataSourceInstanceStateChange::
    DataSourceInstanceStateChange() = default;
ObservableEvents::DataSourceInstanceStateChange::
    ~DataSourceInstanceStateChange() = default;
ObservableEvents::DataSourceInstanceStateChange::DataSourceInstanceStateChange(
    const ObservableEvents::DataSourceInstanceStateChange&) = default;
ObservableEvents::DataSourceInstanceStateChange&
ObservableEvents::DataSourceInstanceStateChange::operator=(
    const ObservableEvents::DataSourceInstanceStateChange&) = default;
ObservableEvents::DataSourceInstanceStateChange::DataSourceInstanceStateChange(
    ObservableEvents::DataSourceInstanceStateChange&&) noexcept = default;
ObservableEvents::DataSourceInstanceStateChange&
ObservableEvents::DataSourceInstanceStateChange::operator=(
    ObservableEvents::DataSourceInstanceStateChange&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool ObservableEvents::DataSourceInstanceStateChange::operator==(
    const ObservableEvents::DataSourceInstanceStateChange& other) const {
  return (producer_name_ == other.producer_name_) &&
         (data_source_name_ == other.data_source_name_) &&
         (state_ == other.state_);
}
#pragma GCC diagnostic pop

void ObservableEvents::DataSourceInstanceStateChange::ParseRawProto(
    const std::string& raw) {
  perfetto::protos::ObservableEvents_DataSourceInstanceStateChange proto;
  proto.ParseFromString(raw);
  FromProto(proto);
}

void ObservableEvents::DataSourceInstanceStateChange::FromProto(
    const perfetto::protos::ObservableEvents_DataSourceInstanceStateChange&
        proto) {
  static_assert(sizeof(producer_name_) == sizeof(proto.producer_name()),
                "size mismatch");
  producer_name_ = static_cast<decltype(producer_name_)>(proto.producer_name());

  static_assert(sizeof(data_source_name_) == sizeof(proto.data_source_name()),
                "size mismatch");
  data_source_name_ =
      static_cast<decltype(data_source_name_)>(proto.data_source_name());

  static_assert(sizeof(state_) == sizeof(proto.state()), "size mismatch");
  state_ = static_cast<decltype(state_)>(proto.state());
  unknown_fields_ = proto.unknown_fields();
}

void ObservableEvents::DataSourceInstanceStateChange::ToProto(
    perfetto::protos::ObservableEvents_DataSourceInstanceStateChange* proto)
    const {
  proto->Clear();

  static_assert(sizeof(producer_name_) == sizeof(proto->producer_name()),
                "size mismatch");
  proto->set_producer_name(
      static_cast<decltype(proto->producer_name())>(producer_name_));

  static_assert(sizeof(data_source_name_) == sizeof(proto->data_source_name()),
                "size mismatch");
  proto->set_data_source_name(
      static_cast<decltype(proto->data_source_name())>(data_source_name_));

  static_assert(sizeof(state_) == sizeof(proto->state()), "size mismatch");
  proto->set_state(static_cast<decltype(proto->state())>(state_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

}  // namespace perfetto
