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
 * perfetto/config/ftrace/ftrace_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef SRC_TRACED_PROBES_FTRACE_FTRACE_CONFIG_H_
#define SRC_TRACED_PROBES_FTRACE_FTRACE_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class FtraceConfig;
}
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT FtraceConfig {
 public:
  FtraceConfig();
  ~FtraceConfig();
  FtraceConfig(FtraceConfig&&) noexcept;
  FtraceConfig& operator=(FtraceConfig&&);
  FtraceConfig(const FtraceConfig&);
  FtraceConfig& operator=(const FtraceConfig&);
  bool operator==(const FtraceConfig&) const;
  bool operator!=(const FtraceConfig& other) const { return !(*this == other); }

  // Raw proto decoding.
  void ParseRawProto(const std::string&);
  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::FtraceConfig&);
  void ToProto(perfetto::protos::FtraceConfig*) const;

  int ftrace_events_size() const {
    return static_cast<int>(ftrace_events_.size());
  }
  const std::vector<std::string>& ftrace_events() const {
    return ftrace_events_;
  }
  std::vector<std::string>* mutable_ftrace_events() { return &ftrace_events_; }
  void clear_ftrace_events() { ftrace_events_.clear(); }
  std::string* add_ftrace_events() {
    ftrace_events_.emplace_back();
    return &ftrace_events_.back();
  }

  int atrace_categories_size() const {
    return static_cast<int>(atrace_categories_.size());
  }
  const std::vector<std::string>& atrace_categories() const {
    return atrace_categories_;
  }
  std::vector<std::string>* mutable_atrace_categories() {
    return &atrace_categories_;
  }
  void clear_atrace_categories() { atrace_categories_.clear(); }
  std::string* add_atrace_categories() {
    atrace_categories_.emplace_back();
    return &atrace_categories_.back();
  }

  int atrace_apps_size() const { return static_cast<int>(atrace_apps_.size()); }
  const std::vector<std::string>& atrace_apps() const { return atrace_apps_; }
  std::vector<std::string>* mutable_atrace_apps() { return &atrace_apps_; }
  void clear_atrace_apps() { atrace_apps_.clear(); }
  std::string* add_atrace_apps() {
    atrace_apps_.emplace_back();
    return &atrace_apps_.back();
  }

  uint32_t buffer_size_kb() const { return buffer_size_kb_; }
  void set_buffer_size_kb(uint32_t value) { buffer_size_kb_ = value; }

  uint32_t drain_period_ms() const { return drain_period_ms_; }
  void set_drain_period_ms(uint32_t value) { drain_period_ms_ = value; }

 private:
  std::vector<std::string> ftrace_events_;
  std::vector<std::string> atrace_categories_;
  std::vector<std::string> atrace_apps_;
  uint32_t buffer_size_kb_ = {};
  uint32_t drain_period_ms_ = {};

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // SRC_TRACED_PROBES_FTRACE_FTRACE_CONFIG_H_
