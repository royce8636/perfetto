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
 * perfetto/common/observable_events.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_OBSERVABLE_EVENTS_H_
#define INCLUDE_PERFETTO_TRACING_CORE_OBSERVABLE_EVENTS_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class ObservableEvents;
class ObservableEvents_DataSourceInstanceStateChange;
}  // namespace protos
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT ObservableEvents {
 public:
  class PERFETTO_EXPORT DataSourceInstanceStateChange {
   public:
    enum DataSourceInstanceState {
      DATA_SOURCE_INSTANCE_STATE_STOPPED = 1,
      DATA_SOURCE_INSTANCE_STATE_STARTED = 2,
    };
    DataSourceInstanceStateChange();
    ~DataSourceInstanceStateChange();
    DataSourceInstanceStateChange(DataSourceInstanceStateChange&&) noexcept;
    DataSourceInstanceStateChange& operator=(DataSourceInstanceStateChange&&);
    DataSourceInstanceStateChange(const DataSourceInstanceStateChange&);
    DataSourceInstanceStateChange& operator=(
        const DataSourceInstanceStateChange&);
    bool operator==(const DataSourceInstanceStateChange&) const;
    bool operator!=(const DataSourceInstanceStateChange& other) const {
      return !(*this == other);
    }

    // Raw proto decoding.
    void ParseRawProto(const std::string&);
    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::
                       ObservableEvents_DataSourceInstanceStateChange&);
    void ToProto(
        perfetto::protos::ObservableEvents_DataSourceInstanceStateChange*)
        const;

    const std::string& producer_name() const { return producer_name_; }
    void set_producer_name(const std::string& value) { producer_name_ = value; }

    const std::string& data_source_name() const { return data_source_name_; }
    void set_data_source_name(const std::string& value) {
      data_source_name_ = value;
    }

    DataSourceInstanceState state() const { return state_; }
    void set_state(DataSourceInstanceState value) { state_ = value; }

   private:
    std::string producer_name_ = {};
    std::string data_source_name_ = {};
    DataSourceInstanceState state_ = {};

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  ObservableEvents();
  ~ObservableEvents();
  ObservableEvents(ObservableEvents&&) noexcept;
  ObservableEvents& operator=(ObservableEvents&&);
  ObservableEvents(const ObservableEvents&);
  ObservableEvents& operator=(const ObservableEvents&);
  bool operator==(const ObservableEvents&) const;
  bool operator!=(const ObservableEvents& other) const {
    return !(*this == other);
  }

  // Raw proto decoding.
  void ParseRawProto(const std::string&);
  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::ObservableEvents&);
  void ToProto(perfetto::protos::ObservableEvents*) const;

  int instance_state_changes_size() const {
    return static_cast<int>(instance_state_changes_.size());
  }
  const std::vector<DataSourceInstanceStateChange>& instance_state_changes()
      const {
    return instance_state_changes_;
  }
  std::vector<DataSourceInstanceStateChange>* mutable_instance_state_changes() {
    return &instance_state_changes_;
  }
  void clear_instance_state_changes() { instance_state_changes_.clear(); }
  DataSourceInstanceStateChange* add_instance_state_changes() {
    instance_state_changes_.emplace_back();
    return &instance_state_changes_.back();
  }

 private:
  std::vector<DataSourceInstanceStateChange> instance_state_changes_;

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_OBSERVABLE_EVENTS_H_
