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

#ifndef INCLUDE_PERFETTO_TRACING_CORE_TRACING_SERVICE_STATE_H_
#define INCLUDE_PERFETTO_TRACING_CORE_TRACING_SERVICE_STATE_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

#include "perfetto/tracing/core/data_source_descriptor.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class TracingServiceState;
class TracingServiceState_Producer;
class TracingServiceState_DataSource;
class DataSourceDescriptor;
}  // namespace protos
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT TracingServiceState {
 public:
  class PERFETTO_EXPORT Producer {
   public:
    Producer();
    ~Producer();
    Producer(Producer&&) noexcept;
    Producer& operator=(Producer&&);
    Producer(const Producer&);
    Producer& operator=(const Producer&);
    bool operator==(const Producer&) const;
    bool operator!=(const Producer& other) const { return !(*this == other); }

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::TracingServiceState_Producer&);
    void ToProto(perfetto::protos::TracingServiceState_Producer*) const;

    int32_t id() const { return id_; }
    void set_id(int32_t value) { id_ = value; }

    const std::string& name() const { return name_; }
    void set_name(const std::string& value) { name_ = value; }

    int32_t uid() const { return uid_; }
    void set_uid(int32_t value) { uid_ = value; }

   private:
    int32_t id_ = {};
    std::string name_ = {};
    int32_t uid_ = {};

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  class PERFETTO_EXPORT DataSource {
   public:
    DataSource();
    ~DataSource();
    DataSource(DataSource&&) noexcept;
    DataSource& operator=(DataSource&&);
    DataSource(const DataSource&);
    DataSource& operator=(const DataSource&);
    bool operator==(const DataSource&) const;
    bool operator!=(const DataSource& other) const { return !(*this == other); }

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::TracingServiceState_DataSource&);
    void ToProto(perfetto::protos::TracingServiceState_DataSource*) const;

    const DataSourceDescriptor& descriptor() const { return descriptor_; }
    DataSourceDescriptor* mutable_descriptor() { return &descriptor_; }

    int32_t producer_id() const { return producer_id_; }
    void set_producer_id(int32_t value) { producer_id_ = value; }

   private:
    DataSourceDescriptor descriptor_ = {};
    int32_t producer_id_ = {};

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  TracingServiceState();
  ~TracingServiceState();
  TracingServiceState(TracingServiceState&&) noexcept;
  TracingServiceState& operator=(TracingServiceState&&);
  TracingServiceState(const TracingServiceState&);
  TracingServiceState& operator=(const TracingServiceState&);
  bool operator==(const TracingServiceState&) const;
  bool operator!=(const TracingServiceState& other) const {
    return !(*this == other);
  }

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::TracingServiceState&);
  void ToProto(perfetto::protos::TracingServiceState*) const;

  int producers_size() const { return static_cast<int>(producers_.size()); }
  const std::vector<Producer>& producers() const { return producers_; }
  std::vector<Producer>* mutable_producers() { return &producers_; }
  void clear_producers() { producers_.clear(); }
  Producer* add_producers() {
    producers_.emplace_back();
    return &producers_.back();
  }

  int data_sources_size() const {
    return static_cast<int>(data_sources_.size());
  }
  const std::vector<DataSource>& data_sources() const { return data_sources_; }
  std::vector<DataSource>* mutable_data_sources() { return &data_sources_; }
  void clear_data_sources() { data_sources_.clear(); }
  DataSource* add_data_sources() {
    data_sources_.emplace_back();
    return &data_sources_.back();
  }

  int32_t num_sessions() const { return num_sessions_; }
  void set_num_sessions(int32_t value) { num_sessions_ = value; }

  int32_t num_sessions_started() const { return num_sessions_started_; }
  void set_num_sessions_started(int32_t value) {
    num_sessions_started_ = value;
  }

 private:
  std::vector<Producer> producers_;
  std::vector<DataSource> data_sources_;
  int32_t num_sessions_ = {};
  int32_t num_sessions_started_ = {};

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_TRACING_SERVICE_STATE_H_
