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
 * perfetto/config/data_source_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#include "perfetto/tracing/core/data_source_config.h"

#include "perfetto/config/android/android_log_config.pb.h"
#include "perfetto/config/chrome/chrome_config.pb.h"
#include "perfetto/config/data_source_config.pb.h"
#include "perfetto/config/ftrace/ftrace_config.pb.h"
#include "perfetto/config/inode_file/inode_file_config.pb.h"
#include "perfetto/config/power/android_power_config.pb.h"
#include "perfetto/config/process_stats/process_stats_config.pb.h"
#include "perfetto/config/profiling/heapprofd_config.pb.h"
#include "perfetto/config/sys_stats/sys_stats_config.pb.h"
#include "perfetto/config/test_config.pb.h"

namespace perfetto {

DataSourceConfig::DataSourceConfig() = default;
DataSourceConfig::~DataSourceConfig() = default;
DataSourceConfig::DataSourceConfig(const DataSourceConfig&) = default;
DataSourceConfig& DataSourceConfig::operator=(const DataSourceConfig&) =
    default;
DataSourceConfig::DataSourceConfig(DataSourceConfig&&) noexcept = default;
DataSourceConfig& DataSourceConfig::operator=(DataSourceConfig&&) = default;

void DataSourceConfig::FromProto(
    const perfetto::protos::DataSourceConfig& proto) {
  static_assert(sizeof(name_) == sizeof(proto.name()), "size mismatch");
  name_ = static_cast<decltype(name_)>(proto.name());

  static_assert(sizeof(target_buffer_) == sizeof(proto.target_buffer()),
                "size mismatch");
  target_buffer_ = static_cast<decltype(target_buffer_)>(proto.target_buffer());

  static_assert(sizeof(trace_duration_ms_) == sizeof(proto.trace_duration_ms()),
                "size mismatch");
  trace_duration_ms_ =
      static_cast<decltype(trace_duration_ms_)>(proto.trace_duration_ms());

  static_assert(
      sizeof(tracing_session_id_) == sizeof(proto.tracing_session_id()),
      "size mismatch");
  tracing_session_id_ =
      static_cast<decltype(tracing_session_id_)>(proto.tracing_session_id());

  ftrace_config_.FromProto(proto.ftrace_config());

  chrome_config_.FromProto(proto.chrome_config());

  inode_file_config_.FromProto(proto.inode_file_config());

  process_stats_config_.FromProto(proto.process_stats_config());

  sys_stats_config_.FromProto(proto.sys_stats_config());

  heapprofd_config_.FromProto(proto.heapprofd_config());

  android_power_config_.FromProto(proto.android_power_config());

  android_log_config_.FromProto(proto.android_log_config());

  static_assert(sizeof(legacy_config_) == sizeof(proto.legacy_config()),
                "size mismatch");
  legacy_config_ = static_cast<decltype(legacy_config_)>(proto.legacy_config());

  for_testing_.FromProto(proto.for_testing());
  unknown_fields_ = proto.unknown_fields();
}

void DataSourceConfig::ToProto(
    perfetto::protos::DataSourceConfig* proto) const {
  proto->Clear();

  static_assert(sizeof(name_) == sizeof(proto->name()), "size mismatch");
  proto->set_name(static_cast<decltype(proto->name())>(name_));

  static_assert(sizeof(target_buffer_) == sizeof(proto->target_buffer()),
                "size mismatch");
  proto->set_target_buffer(
      static_cast<decltype(proto->target_buffer())>(target_buffer_));

  static_assert(
      sizeof(trace_duration_ms_) == sizeof(proto->trace_duration_ms()),
      "size mismatch");
  proto->set_trace_duration_ms(
      static_cast<decltype(proto->trace_duration_ms())>(trace_duration_ms_));

  static_assert(
      sizeof(tracing_session_id_) == sizeof(proto->tracing_session_id()),
      "size mismatch");
  proto->set_tracing_session_id(
      static_cast<decltype(proto->tracing_session_id())>(tracing_session_id_));

  ftrace_config_.ToProto(proto->mutable_ftrace_config());

  chrome_config_.ToProto(proto->mutable_chrome_config());

  inode_file_config_.ToProto(proto->mutable_inode_file_config());

  process_stats_config_.ToProto(proto->mutable_process_stats_config());

  sys_stats_config_.ToProto(proto->mutable_sys_stats_config());

  heapprofd_config_.ToProto(proto->mutable_heapprofd_config());

  android_power_config_.ToProto(proto->mutable_android_power_config());

  android_log_config_.ToProto(proto->mutable_android_log_config());

  static_assert(sizeof(legacy_config_) == sizeof(proto->legacy_config()),
                "size mismatch");
  proto->set_legacy_config(
      static_cast<decltype(proto->legacy_config())>(legacy_config_));

  for_testing_.ToProto(proto->mutable_for_testing());
  *(proto->mutable_unknown_fields()) = unknown_fields_;
}

}  // namespace perfetto
