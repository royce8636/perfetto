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
 * perfetto/config/process_stats/process_stats_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_PROCESS_STATS_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_PROCESS_STATS_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class ProcessStatsConfig;
}
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT ProcessStatsConfig {
 public:
  enum Quirks {
    QUIRKS_UNSPECIFIED = 0,
    DISABLE_INITIAL_DUMP = 1,
    DISABLE_ON_DEMAND = 2,
  };
  ProcessStatsConfig();
  ~ProcessStatsConfig();
  ProcessStatsConfig(ProcessStatsConfig&&) noexcept;
  ProcessStatsConfig& operator=(ProcessStatsConfig&&);
  ProcessStatsConfig(const ProcessStatsConfig&);
  ProcessStatsConfig& operator=(const ProcessStatsConfig&);

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::ProcessStatsConfig&);
  void ToProto(perfetto::protos::ProcessStatsConfig*) const;

  int quirks_size() const { return static_cast<int>(quirks_.size()); }
  const std::vector<Quirks>& quirks() const { return quirks_; }
  Quirks* add_quirks() {
    quirks_.emplace_back();
    return &quirks_.back();
  }

  bool scan_all_processes_on_start() const {
    return scan_all_processes_on_start_;
  }
  void set_scan_all_processes_on_start(bool value) {
    scan_all_processes_on_start_ = value;
  }

  bool record_thread_names() const { return record_thread_names_; }
  void set_record_thread_names(bool value) { record_thread_names_ = value; }

  uint32_t proc_stats_poll_ms() const { return proc_stats_poll_ms_; }
  void set_proc_stats_poll_ms(uint32_t value) { proc_stats_poll_ms_ = value; }

 private:
  std::vector<Quirks> quirks_;
  bool scan_all_processes_on_start_ = {};
  bool record_thread_names_ = {};
  uint32_t proc_stats_poll_ms_ = {};

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_PROCESS_STATS_CONFIG_H_
