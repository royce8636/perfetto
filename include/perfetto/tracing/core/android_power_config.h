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
 * perfetto/config/power/android_power_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_ANDROID_POWER_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_ANDROID_POWER_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class AndroidPowerConfig;
}
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT AndroidPowerConfig {
 public:
  enum BatteryCounters {
    BATTERY_COUNTER_UNSPECIFIED = 0,
    BATTERY_COUNTER_CHARGE = 1,
    BATTERY_COUNTER_CAPACITY_PERCENT = 2,
    BATTERY_COUNTER_CURRENT = 3,
    BATTERY_COUNTER_CURRENT_AVG = 4,
  };
  AndroidPowerConfig();
  ~AndroidPowerConfig();
  AndroidPowerConfig(AndroidPowerConfig&&) noexcept;
  AndroidPowerConfig& operator=(AndroidPowerConfig&&);
  AndroidPowerConfig(const AndroidPowerConfig&);
  AndroidPowerConfig& operator=(const AndroidPowerConfig&);

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::AndroidPowerConfig&);
  void ToProto(perfetto::protos::AndroidPowerConfig*) const;

  uint32_t battery_poll_ms() const { return battery_poll_ms_; }
  void set_battery_poll_ms(uint32_t value) { battery_poll_ms_ = value; }

  int battery_counters_size() const {
    return static_cast<int>(battery_counters_.size());
  }
  const std::vector<BatteryCounters>& battery_counters() const {
    return battery_counters_;
  }
  BatteryCounters* add_battery_counters() {
    battery_counters_.emplace_back();
    return &battery_counters_.back();
  }

  bool collect_power_rails() const { return collect_power_rails_; }
  void set_collect_power_rails(bool value) { collect_power_rails_ = value; }

 private:
  uint32_t battery_poll_ms_ = {};
  std::vector<BatteryCounters> battery_counters_;
  bool collect_power_rails_ = {};

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_ANDROID_POWER_CONFIG_H_
