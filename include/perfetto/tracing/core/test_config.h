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
 * perfetto/config/test_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos.py
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_TEST_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_TEST_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class TestConfig;
}
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT TestConfig {
 public:
  TestConfig();
  ~TestConfig();
  TestConfig(TestConfig&&) noexcept;
  TestConfig& operator=(TestConfig&&);
  TestConfig(const TestConfig&);
  TestConfig& operator=(const TestConfig&);

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::TestConfig&);
  void ToProto(perfetto::protos::TestConfig*) const;

  uint32_t message_count() const { return message_count_; }
  void set_message_count(uint32_t value) { message_count_ = value; }

  uint32_t seed() const { return seed_; }
  void set_seed(uint32_t value) { seed_ = value; }

  uint64_t message_size() const { return message_size_; }
  void set_message_size(uint64_t value) { message_size_ = value; }

 private:
  uint32_t message_count_ = {};
  uint32_t seed_ = {};
  uint64_t message_size_ = {};

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto
#endif  // INCLUDE_PERFETTO_TRACING_CORE_TEST_CONFIG_H_
