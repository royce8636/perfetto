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
 * perfetto/config/android/android_log_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_ANDROID_LOG_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_ANDROID_LOG_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

#include "perfetto/tracing/core/android_log_constants.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class AndroidLogConfig;
}
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT AndroidLogConfig {
 public:
  enum AndroidLogId {
    LID_DEFAULT = 0,
    LID_RADIO = 1,
    LID_EVENTS = 2,
    LID_SYSTEM = 3,
    LID_CRASH = 4,
    LID_STATS = 5,
    LID_SECURITY = 6,
    LID_KERNEL = 7,
  };
  enum AndroidLogPriority {
    PRIO_UNSPECIFIED = 0,
    PRIO_UNUSED = 1,
    PRIO_VERBOSE = 2,
    PRIO_DEBUG = 3,
    PRIO_INFO = 4,
    PRIO_WARN = 5,
    PRIO_ERROR = 6,
    PRIO_FATAL = 7,
  };
  AndroidLogConfig();
  ~AndroidLogConfig();
  AndroidLogConfig(AndroidLogConfig&&) noexcept;
  AndroidLogConfig& operator=(AndroidLogConfig&&);
  AndroidLogConfig(const AndroidLogConfig&);
  AndroidLogConfig& operator=(const AndroidLogConfig&);
  bool operator==(const AndroidLogConfig&) const;
  bool operator!=(const AndroidLogConfig& other) const {
    return !(*this == other);
  }

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::AndroidLogConfig&);
  void ToProto(perfetto::protos::AndroidLogConfig*) const;

  int log_ids_size() const { return static_cast<int>(log_ids_.size()); }
  const std::vector<AndroidLogId>& log_ids() const { return log_ids_; }
  std::vector<AndroidLogId>* mutable_log_ids() { return &log_ids_; }
  void clear_log_ids() { log_ids_.clear(); }
  AndroidLogId* add_log_ids() {
    log_ids_.emplace_back();
    return &log_ids_.back();
  }

  AndroidLogPriority min_prio() const { return min_prio_; }
  void set_min_prio(AndroidLogPriority value) { min_prio_ = value; }

  int filter_tags_size() const { return static_cast<int>(filter_tags_.size()); }
  const std::vector<std::string>& filter_tags() const { return filter_tags_; }
  std::vector<std::string>* mutable_filter_tags() { return &filter_tags_; }
  void clear_filter_tags() { filter_tags_.clear(); }
  std::string* add_filter_tags() {
    filter_tags_.emplace_back();
    return &filter_tags_.back();
  }

 private:
  std::vector<AndroidLogId> log_ids_;
  AndroidLogPriority min_prio_ = {};
  std::vector<std::string> filter_tags_;

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_ANDROID_LOG_CONFIG_H_
