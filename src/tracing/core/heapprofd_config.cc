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
 * perfetto/config/profiling/heapprofd_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#include "perfetto/tracing/core/heapprofd_config.h"

#include "perfetto/config/profiling/heapprofd_config.pb.h"

namespace perfetto {

HeapprofdConfig::HeapprofdConfig() = default;
HeapprofdConfig::~HeapprofdConfig() = default;
HeapprofdConfig::HeapprofdConfig(const HeapprofdConfig&) = default;
HeapprofdConfig& HeapprofdConfig::operator=(const HeapprofdConfig&) = default;
HeapprofdConfig::HeapprofdConfig(HeapprofdConfig&&) noexcept = default;
HeapprofdConfig& HeapprofdConfig::operator=(HeapprofdConfig&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool HeapprofdConfig::operator==(const HeapprofdConfig& other) const {
  return (sampling_interval_bytes_ == other.sampling_interval_bytes_) &&
         (process_cmdline_ == other.process_cmdline_) && (pid_ == other.pid_) &&
         (all_ == other.all_) &&
         (skip_symbol_prefix_ == other.skip_symbol_prefix_) &&
         (continuous_dump_config_ == other.continuous_dump_config_) &&
         (shmem_size_bytes_ == other.shmem_size_bytes_);
}
#pragma GCC diagnostic pop

void HeapprofdConfig::FromProto(
    const perfetto::protos::HeapprofdConfig& proto) {
  static_assert(sizeof(sampling_interval_bytes_) ==
                    sizeof(proto.sampling_interval_bytes()),
                "size mismatch");
  sampling_interval_bytes_ = static_cast<decltype(sampling_interval_bytes_)>(
      proto.sampling_interval_bytes());

  process_cmdline_.clear();
  for (const auto& field : proto.process_cmdline()) {
    process_cmdline_.emplace_back();
    static_assert(
        sizeof(process_cmdline_.back()) == sizeof(proto.process_cmdline(0)),
        "size mismatch");
    process_cmdline_.back() =
        static_cast<decltype(process_cmdline_)::value_type>(field);
  }

  pid_.clear();
  for (const auto& field : proto.pid()) {
    pid_.emplace_back();
    static_assert(sizeof(pid_.back()) == sizeof(proto.pid(0)), "size mismatch");
    pid_.back() = static_cast<decltype(pid_)::value_type>(field);
  }

  static_assert(sizeof(all_) == sizeof(proto.all()), "size mismatch");
  all_ = static_cast<decltype(all_)>(proto.all());

  skip_symbol_prefix_.clear();
  for (const auto& field : proto.skip_symbol_prefix()) {
    skip_symbol_prefix_.emplace_back();
    static_assert(sizeof(skip_symbol_prefix_.back()) ==
                      sizeof(proto.skip_symbol_prefix(0)),
                  "size mismatch");
    skip_symbol_prefix_.back() =
        static_cast<decltype(skip_symbol_prefix_)::value_type>(field);
  }

  continuous_dump_config_.FromProto(proto.continuous_dump_config());

  static_assert(sizeof(shmem_size_bytes_) == sizeof(proto.shmem_size_bytes()),
                "size mismatch");
  shmem_size_bytes_ =
      static_cast<decltype(shmem_size_bytes_)>(proto.shmem_size_bytes());
  unknown_fields_ = proto.unknown_fields();
}

void HeapprofdConfig::ToProto(perfetto::protos::HeapprofdConfig* proto) const {
  proto->Clear();

  static_assert(sizeof(sampling_interval_bytes_) ==
                    sizeof(proto->sampling_interval_bytes()),
                "size mismatch");
  proto->set_sampling_interval_bytes(
      static_cast<decltype(proto->sampling_interval_bytes())>(
          sampling_interval_bytes_));

  for (const auto& it : process_cmdline_) {
    proto->add_process_cmdline(
        static_cast<decltype(proto->process_cmdline(0))>(it));
    static_assert(sizeof(it) == sizeof(proto->process_cmdline(0)),
                  "size mismatch");
  }

  for (const auto& it : pid_) {
    proto->add_pid(static_cast<decltype(proto->pid(0))>(it));
    static_assert(sizeof(it) == sizeof(proto->pid(0)), "size mismatch");
  }

  static_assert(sizeof(all_) == sizeof(proto->all()), "size mismatch");
  proto->set_all(static_cast<decltype(proto->all())>(all_));

  for (const auto& it : skip_symbol_prefix_) {
    proto->add_skip_symbol_prefix(
        static_cast<decltype(proto->skip_symbol_prefix(0))>(it));
    static_assert(sizeof(it) == sizeof(proto->skip_symbol_prefix(0)),
                  "size mismatch");
  }

  continuous_dump_config_.ToProto(proto->mutable_continuous_dump_config());

  static_assert(sizeof(shmem_size_bytes_) == sizeof(proto->shmem_size_bytes()),
                "size mismatch");
  proto->set_shmem_size_bytes(
      static_cast<decltype(proto->shmem_size_bytes())>(shmem_size_bytes_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

HeapprofdConfig::ContinuousDumpConfig::ContinuousDumpConfig() = default;
HeapprofdConfig::ContinuousDumpConfig::~ContinuousDumpConfig() = default;
HeapprofdConfig::ContinuousDumpConfig::ContinuousDumpConfig(
    const HeapprofdConfig::ContinuousDumpConfig&) = default;
HeapprofdConfig::ContinuousDumpConfig& HeapprofdConfig::ContinuousDumpConfig::
operator=(const HeapprofdConfig::ContinuousDumpConfig&) = default;
HeapprofdConfig::ContinuousDumpConfig::ContinuousDumpConfig(
    HeapprofdConfig::ContinuousDumpConfig&&) noexcept = default;
HeapprofdConfig::ContinuousDumpConfig& HeapprofdConfig::ContinuousDumpConfig::
operator=(HeapprofdConfig::ContinuousDumpConfig&&) = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
bool HeapprofdConfig::ContinuousDumpConfig::operator==(
    const HeapprofdConfig::ContinuousDumpConfig& other) const {
  return (dump_phase_ms_ == other.dump_phase_ms_) &&
         (dump_interval_ms_ == other.dump_interval_ms_);
}
#pragma GCC diagnostic pop

void HeapprofdConfig::ContinuousDumpConfig::FromProto(
    const perfetto::protos::HeapprofdConfig_ContinuousDumpConfig& proto) {
  static_assert(sizeof(dump_phase_ms_) == sizeof(proto.dump_phase_ms()),
                "size mismatch");
  dump_phase_ms_ = static_cast<decltype(dump_phase_ms_)>(proto.dump_phase_ms());

  static_assert(sizeof(dump_interval_ms_) == sizeof(proto.dump_interval_ms()),
                "size mismatch");
  dump_interval_ms_ =
      static_cast<decltype(dump_interval_ms_)>(proto.dump_interval_ms());
  unknown_fields_ = proto.unknown_fields();
}

void HeapprofdConfig::ContinuousDumpConfig::ToProto(
    perfetto::protos::HeapprofdConfig_ContinuousDumpConfig* proto) const {
  proto->Clear();

  static_assert(sizeof(dump_phase_ms_) == sizeof(proto->dump_phase_ms()),
                "size mismatch");
  proto->set_dump_phase_ms(
      static_cast<decltype(proto->dump_phase_ms())>(dump_phase_ms_));

  static_assert(sizeof(dump_interval_ms_) == sizeof(proto->dump_interval_ms()),
                "size mismatch");
  proto->set_dump_interval_ms(
      static_cast<decltype(proto->dump_interval_ms())>(dump_interval_ms_));
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

}  // namespace perfetto
