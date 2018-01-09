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
 * protos/tracing_service/trace_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos.py
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_TRACE_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_TRACE_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>
#include "perfetto/base/build_config.h"

#include "perfetto/tracing/core/data_source_config.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class TraceConfig;
class TraceConfig_BufferConfig;
class TraceConfig_DataSource;
class DataSourceConfig;
class DataSourceConfig_FtraceConfig;
}  // namespace protos
}  // namespace perfetto

namespace perfetto {

class TraceConfig {
 public:
  class BufferConfig {
   public:
    enum OptimizeFor {
      DEFAULT = 0,
      ONE_SHOT_READ = 1,
    };
    enum FillPolicy {
      UNSPECIFIED = 0,
      RING_BUFFER = 1,
    };
    BufferConfig();
    ~BufferConfig();
    BufferConfig(BufferConfig&&) noexcept;
    BufferConfig& operator=(BufferConfig&&);
    BufferConfig(const BufferConfig&);
    BufferConfig& operator=(const BufferConfig&);

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::TraceConfig_BufferConfig&);
    void ToProto(perfetto::protos::TraceConfig_BufferConfig*) const;

    uint32_t size_kb() const { return size_kb_; }
    void set_size_kb(uint32_t value) { size_kb_ = value; }

    OptimizeFor optimize_for() const { return optimize_for_; }
    void set_optimize_for(OptimizeFor value) { optimize_for_ = value; }

    FillPolicy fill_policy() const { return fill_policy_; }
    void set_fill_policy(FillPolicy value) { fill_policy_ = value; }

   private:
    uint32_t size_kb_ = {};
    OptimizeFor optimize_for_ = {};
    FillPolicy fill_policy_ = {};

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  class DataSource {
   public:
    DataSource();
    ~DataSource();
    DataSource(DataSource&&) noexcept;
    DataSource& operator=(DataSource&&);
    DataSource(const DataSource&);
    DataSource& operator=(const DataSource&);

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::TraceConfig_DataSource&);
    void ToProto(perfetto::protos::TraceConfig_DataSource*) const;

    const DataSourceConfig& config() const { return config_; }
    DataSourceConfig* mutable_config() { return &config_; }

    int producer_name_filter_size() const {
      return static_cast<int>(producer_name_filter_.size());
    }
    const std::vector<std::string>& producer_name_filter() const {
      return producer_name_filter_;
    }
    std::string* add_producer_name_filter() {
      producer_name_filter_.emplace_back();
      return &producer_name_filter_.back();
    }

   private:
    DataSourceConfig config_ = {};
    std::vector<std::string> producer_name_filter_;

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  TraceConfig();
  ~TraceConfig();
  TraceConfig(TraceConfig&&) noexcept;
  TraceConfig& operator=(TraceConfig&&);
  TraceConfig(const TraceConfig&);
  TraceConfig& operator=(const TraceConfig&);

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::TraceConfig&);
  void ToProto(perfetto::protos::TraceConfig*) const;

  int buffers_size() const { return static_cast<int>(buffers_.size()); }
  const std::vector<BufferConfig>& buffers() const { return buffers_; }
  BufferConfig* add_buffers() {
    buffers_.emplace_back();
    return &buffers_.back();
  }

  int data_sources_size() const {
    return static_cast<int>(data_sources_.size());
  }
  const std::vector<DataSource>& data_sources() const { return data_sources_; }
  DataSource* add_data_sources() {
    data_sources_.emplace_back();
    return &data_sources_.back();
  }

  uint32_t duration_ms() const { return duration_ms_; }
  void set_duration_ms(uint32_t value) { duration_ms_ = value; }

 private:
  std::vector<BufferConfig> buffers_;
  std::vector<DataSource> data_sources_;
  uint32_t duration_ms_ = {};

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto
#endif  // INCLUDE_PERFETTO_TRACING_CORE_TRACE_CONFIG_H_
