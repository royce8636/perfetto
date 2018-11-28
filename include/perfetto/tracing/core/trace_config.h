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
 * perfetto/config/trace_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_TRACE_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_TRACE_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

#include "perfetto/tracing/core/data_source_config.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class TraceConfig;
class TraceConfig_BufferConfig;
class TraceConfig_DataSource;
class DataSourceConfig;
class FtraceConfig;
class ChromeConfig;
class InodeFileConfig;
class InodeFileConfig_MountPointMappingEntry;
class ProcessStatsConfig;
class SysStatsConfig;
class HeapprofdConfig;
class HeapprofdConfig_ContinousDumpConfig;
class TestConfig;
class TestConfig_DummyFields;
class TraceConfig_ProducerConfig;
class TraceConfig_StatsdMetadata;
class TraceConfig_GuardrailOverrides;
}  // namespace protos
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT TraceConfig {
 public:
  class PERFETTO_EXPORT BufferConfig {
   public:
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

    FillPolicy fill_policy() const { return fill_policy_; }
    void set_fill_policy(FillPolicy value) { fill_policy_ = value; }

   private:
    uint32_t size_kb_ = {};
    FillPolicy fill_policy_ = {};

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

  enum LockdownModeOperation {
    LOCKDOWN_UNCHANGED = 0,
    LOCKDOWN_CLEAR = 1,
    LOCKDOWN_SET = 2,
  };

  class PERFETTO_EXPORT ProducerConfig {
   public:
    ProducerConfig();
    ~ProducerConfig();
    ProducerConfig(ProducerConfig&&) noexcept;
    ProducerConfig& operator=(ProducerConfig&&);
    ProducerConfig(const ProducerConfig&);
    ProducerConfig& operator=(const ProducerConfig&);

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::TraceConfig_ProducerConfig&);
    void ToProto(perfetto::protos::TraceConfig_ProducerConfig*) const;

    const std::string& producer_name() const { return producer_name_; }
    void set_producer_name(const std::string& value) { producer_name_ = value; }

    uint32_t shm_size_kb() const { return shm_size_kb_; }
    void set_shm_size_kb(uint32_t value) { shm_size_kb_ = value; }

    uint32_t page_size_kb() const { return page_size_kb_; }
    void set_page_size_kb(uint32_t value) { page_size_kb_ = value; }

   private:
    std::string producer_name_ = {};
    uint32_t shm_size_kb_ = {};
    uint32_t page_size_kb_ = {};

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  class PERFETTO_EXPORT StatsdMetadata {
   public:
    StatsdMetadata();
    ~StatsdMetadata();
    StatsdMetadata(StatsdMetadata&&) noexcept;
    StatsdMetadata& operator=(StatsdMetadata&&);
    StatsdMetadata(const StatsdMetadata&);
    StatsdMetadata& operator=(const StatsdMetadata&);

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::TraceConfig_StatsdMetadata&);
    void ToProto(perfetto::protos::TraceConfig_StatsdMetadata*) const;

    int64_t triggering_alert_id() const { return triggering_alert_id_; }
    void set_triggering_alert_id(int64_t value) {
      triggering_alert_id_ = value;
    }

    int32_t triggering_config_uid() const { return triggering_config_uid_; }
    void set_triggering_config_uid(int32_t value) {
      triggering_config_uid_ = value;
    }

    int64_t triggering_config_id() const { return triggering_config_id_; }
    void set_triggering_config_id(int64_t value) {
      triggering_config_id_ = value;
    }

   private:
    int64_t triggering_alert_id_ = {};
    int32_t triggering_config_uid_ = {};
    int64_t triggering_config_id_ = {};

    // Allows to preserve unknown protobuf fields for compatibility
    // with future versions of .proto files.
    std::string unknown_fields_;
  };

  class PERFETTO_EXPORT GuardrailOverrides {
   public:
    GuardrailOverrides();
    ~GuardrailOverrides();
    GuardrailOverrides(GuardrailOverrides&&) noexcept;
    GuardrailOverrides& operator=(GuardrailOverrides&&);
    GuardrailOverrides(const GuardrailOverrides&);
    GuardrailOverrides& operator=(const GuardrailOverrides&);

    // Conversion methods from/to the corresponding protobuf types.
    void FromProto(const perfetto::protos::TraceConfig_GuardrailOverrides&);
    void ToProto(perfetto::protos::TraceConfig_GuardrailOverrides*) const;

    uint64_t max_upload_per_day_bytes() const {
      return max_upload_per_day_bytes_;
    }
    void set_max_upload_per_day_bytes(uint64_t value) {
      max_upload_per_day_bytes_ = value;
    }

   private:
    uint64_t max_upload_per_day_bytes_ = {};

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

  bool enable_extra_guardrails() const { return enable_extra_guardrails_; }
  void set_enable_extra_guardrails(bool value) {
    enable_extra_guardrails_ = value;
  }

  LockdownModeOperation lockdown_mode() const { return lockdown_mode_; }
  void set_lockdown_mode(LockdownModeOperation value) {
    lockdown_mode_ = value;
  }

  int producers_size() const { return static_cast<int>(producers_.size()); }
  const std::vector<ProducerConfig>& producers() const { return producers_; }
  ProducerConfig* add_producers() {
    producers_.emplace_back();
    return &producers_.back();
  }

  const StatsdMetadata& statsd_metadata() const { return statsd_metadata_; }
  StatsdMetadata* mutable_statsd_metadata() { return &statsd_metadata_; }

  bool write_into_file() const { return write_into_file_; }
  void set_write_into_file(bool value) { write_into_file_ = value; }

  uint32_t file_write_period_ms() const { return file_write_period_ms_; }
  void set_file_write_period_ms(uint32_t value) {
    file_write_period_ms_ = value;
  }

  uint64_t max_file_size_bytes() const { return max_file_size_bytes_; }
  void set_max_file_size_bytes(uint64_t value) { max_file_size_bytes_ = value; }

  const GuardrailOverrides& guardrail_overrides() const {
    return guardrail_overrides_;
  }
  GuardrailOverrides* mutable_guardrail_overrides() {
    return &guardrail_overrides_;
  }

  bool deferred_start() const { return deferred_start_; }
  void set_deferred_start(bool value) { deferred_start_ = value; }

  uint32_t flush_period_ms() const { return flush_period_ms_; }
  void set_flush_period_ms(uint32_t value) { flush_period_ms_ = value; }

 private:
  std::vector<BufferConfig> buffers_;
  std::vector<DataSource> data_sources_;
  uint32_t duration_ms_ = {};
  bool enable_extra_guardrails_ = {};
  LockdownModeOperation lockdown_mode_ = {};
  std::vector<ProducerConfig> producers_;
  StatsdMetadata statsd_metadata_ = {};
  bool write_into_file_ = {};
  uint32_t file_write_period_ms_ = {};
  uint64_t max_file_size_bytes_ = {};
  GuardrailOverrides guardrail_overrides_ = {};
  bool deferred_start_ = {};
  uint32_t flush_period_ms_ = {};

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_TRACE_CONFIG_H_
