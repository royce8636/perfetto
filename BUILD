# Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is automatically generated by tools/gen_build. Do not edit.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

alias(
    name = "libprotozero",
    actual = ":src_protozero_libprotozero",
)

alias(
    name = "protozero_plugin",
    actual = ":src_protozero_protoc_plugin_protoc_plugin",
)

# GN target: //src/protozero:libprotozero
cc_library(
    name = "src_protozero_libprotozero",
    srcs = [
        "src/base/event.cc",
        "src/base/file_utils.cc",
        "src/base/metatrace.cc",
        "src/base/paged_memory.cc",
        "src/base/pipe.cc",
        "src/base/string_splitter.cc",
        "src/base/string_utils.cc",
        "src/base/string_view.cc",
        "src/base/temp_file.cc",
        "src/base/thread_checker.cc",
        "src/base/thread_task_runner.cc",
        "src/base/time.cc",
        "src/base/unix_task_runner.cc",
        "src/base/virtual_destructors.cc",
        "src/base/watchdog_posix.cc",
        "src/protozero/message.cc",
        "src/protozero/message_handle.cc",
        "src/protozero/proto_decoder.cc",
        "src/protozero/scattered_heap_buffer.cc",
        "src/protozero/scattered_stream_null_delegate.cc",
        "src/protozero/scattered_stream_writer.cc",
    ],
    hdrs = [
        "include/perfetto/base/build_config.h",
        "include/perfetto/base/circular_queue.h",
        "include/perfetto/base/container_annotations.h",
        "include/perfetto/base/event.h",
        "include/perfetto/base/export.h",
        "include/perfetto/base/file_utils.h",
        "include/perfetto/base/gtest_prod_util.h",
        "include/perfetto/base/hash.h",
        "include/perfetto/base/logging.h",
        "include/perfetto/base/metatrace.h",
        "include/perfetto/base/optional.h",
        "include/perfetto/base/paged_memory.h",
        "include/perfetto/base/pipe.h",
        "include/perfetto/base/scoped_file.h",
        "include/perfetto/base/small_set.h",
        "include/perfetto/base/string_splitter.h",
        "include/perfetto/base/string_utils.h",
        "include/perfetto/base/string_view.h",
        "include/perfetto/base/string_writer.h",
        "include/perfetto/base/task_runner.h",
        "include/perfetto/base/temp_file.h",
        "include/perfetto/base/thread_checker.h",
        "include/perfetto/base/thread_task_runner.h",
        "include/perfetto/base/thread_utils.h",
        "include/perfetto/base/time.h",
        "include/perfetto/base/unix_socket.h",
        "include/perfetto/base/unix_task_runner.h",
        "include/perfetto/base/utils.h",
        "include/perfetto/base/watchdog.h",
        "include/perfetto/base/watchdog_noop.h",
        "include/perfetto/base/watchdog_posix.h",
        "include/perfetto/base/weak_ptr.h",
        "include/perfetto/protozero/contiguous_memory_range.h",
        "include/perfetto/protozero/field.h",
        "include/perfetto/protozero/message.h",
        "include/perfetto/protozero/message_handle.h",
        "include/perfetto/protozero/proto_decoder.h",
        "include/perfetto/protozero/proto_utils.h",
        "include/perfetto/protozero/scattered_heap_buffer.h",
        "include/perfetto/protozero/scattered_stream_null_delegate.h",
        "include/perfetto/protozero/scattered_stream_writer.h",
    ],
    deps = [
        "//third_party/perfetto/google:gtest_prod",
    ],
)

# GN target:
# //src/protozero/protoc_plugin:protoc_plugin(//gn/standalone/toolchain:gcc_like_host)
cc_binary(
    name = "src_protozero_protoc_plugin_protoc_plugin",
    srcs = [
        "src/protozero/protoc_plugin/protozero_generator.cc",
        "src/protozero/protoc_plugin/protozero_generator.h",
        "src/protozero/protoc_plugin/protozero_plugin.cc",
    ],
    deps = [
        "//third_party/protobuf",
        "//third_party/protobuf:libprotoc",
    ],
)

# GN target: //src/trace_processor:trace_processor
cc_library(
    name = "src_trace_processor_trace_processor",
    srcs = [
        "src/base/event.cc",
        "src/base/file_utils.cc",
        "src/base/metatrace.cc",
        "src/base/paged_memory.cc",
        "src/base/pipe.cc",
        "src/base/string_splitter.cc",
        "src/base/string_utils.cc",
        "src/base/string_view.cc",
        "src/base/temp_file.cc",
        "src/base/thread_checker.cc",
        "src/base/thread_task_runner.cc",
        "src/base/time.cc",
        "src/base/unix_task_runner.cc",
        "src/base/virtual_destructors.cc",
        "src/base/watchdog_posix.cc",
        "src/protozero/message.cc",
        "src/protozero/message_handle.cc",
        "src/protozero/proto_decoder.cc",
        "src/protozero/scattered_heap_buffer.cc",
        "src/protozero/scattered_stream_null_delegate.cc",
        "src/protozero/scattered_stream_writer.cc",
        "src/trace_processor/android_logs_table.cc",
        "src/trace_processor/android_logs_table.h",
        "src/trace_processor/args_table.cc",
        "src/trace_processor/args_table.h",
        "src/trace_processor/args_tracker.cc",
        "src/trace_processor/args_tracker.h",
        "src/trace_processor/chunked_trace_reader.h",
        "src/trace_processor/clock_tracker.cc",
        "src/trace_processor/clock_tracker.h",
        "src/trace_processor/counter_definitions_table.cc",
        "src/trace_processor/counter_definitions_table.h",
        "src/trace_processor/counter_values_table.cc",
        "src/trace_processor/counter_values_table.h",
        "src/trace_processor/event_tracker.cc",
        "src/trace_processor/event_tracker.h",
        "src/trace_processor/filtered_row_index.cc",
        "src/trace_processor/filtered_row_index.h",
        "src/trace_processor/ftrace_descriptors.cc",
        "src/trace_processor/ftrace_descriptors.h",
        "src/trace_processor/ftrace_utils.cc",
        "src/trace_processor/ftrace_utils.h",
        "src/trace_processor/instants_table.cc",
        "src/trace_processor/instants_table.h",
        "src/trace_processor/json_trace_parser.cc",
        "src/trace_processor/json_trace_parser.h",
        "src/trace_processor/null_term_string_view.h",
        "src/trace_processor/process_table.cc",
        "src/trace_processor/process_table.h",
        "src/trace_processor/process_tracker.cc",
        "src/trace_processor/process_tracker.h",
        "src/trace_processor/proto_trace_parser.cc",
        "src/trace_processor/proto_trace_parser.h",
        "src/trace_processor/proto_trace_tokenizer.cc",
        "src/trace_processor/proto_trace_tokenizer.h",
        "src/trace_processor/query_constraints.cc",
        "src/trace_processor/query_constraints.h",
        "src/trace_processor/raw_table.cc",
        "src/trace_processor/raw_table.h",
        "src/trace_processor/row_iterators.cc",
        "src/trace_processor/row_iterators.h",
        "src/trace_processor/sched_slice_table.cc",
        "src/trace_processor/sched_slice_table.h",
        "src/trace_processor/scoped_db.h",
        "src/trace_processor/slice_table.cc",
        "src/trace_processor/slice_table.h",
        "src/trace_processor/slice_tracker.cc",
        "src/trace_processor/slice_tracker.h",
        "src/trace_processor/span_join_operator_table.cc",
        "src/trace_processor/span_join_operator_table.h",
        "src/trace_processor/sql_stats_table.cc",
        "src/trace_processor/sql_stats_table.h",
        "src/trace_processor/sqlite3_str_split.cc",
        "src/trace_processor/sqlite3_str_split.h",
        "src/trace_processor/sqlite_utils.h",
        "src/trace_processor/stats.h",
        "src/trace_processor/stats_table.cc",
        "src/trace_processor/stats_table.h",
        "src/trace_processor/storage_columns.cc",
        "src/trace_processor/storage_columns.h",
        "src/trace_processor/storage_schema.cc",
        "src/trace_processor/storage_schema.h",
        "src/trace_processor/storage_table.cc",
        "src/trace_processor/storage_table.h",
        "src/trace_processor/string_pool.cc",
        "src/trace_processor/string_pool.h",
        "src/trace_processor/string_table.cc",
        "src/trace_processor/string_table.h",
        "src/trace_processor/syscall_tracker.cc",
        "src/trace_processor/syscall_tracker.h",
        "src/trace_processor/table.cc",
        "src/trace_processor/table.h",
        "src/trace_processor/thread_table.cc",
        "src/trace_processor/thread_table.h",
        "src/trace_processor/trace_blob_view.h",
        "src/trace_processor/trace_processor.cc",
        "src/trace_processor/trace_processor_context.cc",
        "src/trace_processor/trace_processor_context.h",
        "src/trace_processor/trace_processor_impl.cc",
        "src/trace_processor/trace_processor_impl.h",
        "src/trace_processor/trace_sorter.cc",
        "src/trace_processor/trace_sorter.h",
        "src/trace_processor/trace_storage.cc",
        "src/trace_processor/trace_storage.h",
        "src/trace_processor/virtual_destructors.cc",
        "src/trace_processor/window_operator_table.cc",
        "src/trace_processor/window_operator_table.h",
    ],
    hdrs = [
        "include/perfetto/base/build_config.h",
        "include/perfetto/base/circular_queue.h",
        "include/perfetto/base/container_annotations.h",
        "include/perfetto/base/event.h",
        "include/perfetto/base/export.h",
        "include/perfetto/base/file_utils.h",
        "include/perfetto/base/gtest_prod_util.h",
        "include/perfetto/base/hash.h",
        "include/perfetto/base/logging.h",
        "include/perfetto/base/metatrace.h",
        "include/perfetto/base/optional.h",
        "include/perfetto/base/paged_memory.h",
        "include/perfetto/base/pipe.h",
        "include/perfetto/base/scoped_file.h",
        "include/perfetto/base/small_set.h",
        "include/perfetto/base/string_splitter.h",
        "include/perfetto/base/string_utils.h",
        "include/perfetto/base/string_view.h",
        "include/perfetto/base/string_writer.h",
        "include/perfetto/base/task_runner.h",
        "include/perfetto/base/temp_file.h",
        "include/perfetto/base/thread_checker.h",
        "include/perfetto/base/thread_task_runner.h",
        "include/perfetto/base/thread_utils.h",
        "include/perfetto/base/time.h",
        "include/perfetto/base/unix_socket.h",
        "include/perfetto/base/unix_task_runner.h",
        "include/perfetto/base/utils.h",
        "include/perfetto/base/watchdog.h",
        "include/perfetto/base/watchdog_noop.h",
        "include/perfetto/base/watchdog_posix.h",
        "include/perfetto/base/weak_ptr.h",
        "include/perfetto/protozero/contiguous_memory_range.h",
        "include/perfetto/protozero/field.h",
        "include/perfetto/protozero/message.h",
        "include/perfetto/protozero/message_handle.h",
        "include/perfetto/protozero/proto_decoder.h",
        "include/perfetto/protozero/proto_utils.h",
        "include/perfetto/protozero/scattered_heap_buffer.h",
        "include/perfetto/protozero/scattered_stream_null_delegate.h",
        "include/perfetto/protozero/scattered_stream_writer.h",
        "include/perfetto/trace_processor/basic_types.h",
        "include/perfetto/trace_processor/trace_processor.h",
        "include/perfetto/traced/sys_stats_counters.h",
    ],
    deps = [
        "//third_party/perfetto/google:gtest_prod",
        "//third_party/perfetto/google:jsoncpp",
        "//third_party/perfetto/protos:android_zero_cc_proto",
        "//third_party/perfetto/protos:chrome_zero_cc_proto",
        "//third_party/perfetto/protos:common_cc_proto",
        "//third_party/perfetto/protos:common_zero_cc_proto",
        "//third_party/perfetto/protos:config_zero_cc_proto",
        "//third_party/perfetto/protos:filesystem_zero_cc_proto",
        "//third_party/perfetto/protos:ftrace_zero_cc_proto",
        "//third_party/perfetto/protos:interned_data_zero_cc_proto",
        "//third_party/perfetto/protos:power_zero_cc_proto",
        "//third_party/perfetto/protos:profiling_zero_cc_proto",
        "//third_party/perfetto/protos:ps_zero_cc_proto",
        "//third_party/perfetto/protos:sys_stats_zero_cc_proto",
        "//third_party/perfetto/protos:trace_processor_cc_proto",
        "//third_party/perfetto/protos:trace_zero_cc_proto",
        "//third_party/perfetto/protos:track_event_zero_cc_proto",
        "//third_party/sqlite",
        "//third_party/sqlite:sqlite_ext_percentile",
    ],
)

# GN target:
# //src/trace_processor:trace_processor_shell_host(//gn/standalone/toolchain:gcc_like_host)
cc_binary(
    name = "src_trace_processor_trace_processor_shell_host",
    srcs = [
        "include/perfetto/base/build_config.h",
        "include/perfetto/base/circular_queue.h",
        "include/perfetto/base/container_annotations.h",
        "include/perfetto/base/event.h",
        "include/perfetto/base/export.h",
        "include/perfetto/base/file_utils.h",
        "include/perfetto/base/gtest_prod_util.h",
        "include/perfetto/base/hash.h",
        "include/perfetto/base/logging.h",
        "include/perfetto/base/metatrace.h",
        "include/perfetto/base/optional.h",
        "include/perfetto/base/paged_memory.h",
        "include/perfetto/base/pipe.h",
        "include/perfetto/base/scoped_file.h",
        "include/perfetto/base/small_set.h",
        "include/perfetto/base/string_splitter.h",
        "include/perfetto/base/string_utils.h",
        "include/perfetto/base/string_view.h",
        "include/perfetto/base/string_writer.h",
        "include/perfetto/base/task_runner.h",
        "include/perfetto/base/temp_file.h",
        "include/perfetto/base/thread_checker.h",
        "include/perfetto/base/thread_task_runner.h",
        "include/perfetto/base/thread_utils.h",
        "include/perfetto/base/time.h",
        "include/perfetto/base/unix_socket.h",
        "include/perfetto/base/unix_task_runner.h",
        "include/perfetto/base/utils.h",
        "include/perfetto/base/watchdog.h",
        "include/perfetto/base/watchdog_noop.h",
        "include/perfetto/base/watchdog_posix.h",
        "include/perfetto/base/weak_ptr.h",
        "include/perfetto/protozero/contiguous_memory_range.h",
        "include/perfetto/protozero/field.h",
        "include/perfetto/protozero/message.h",
        "include/perfetto/protozero/message_handle.h",
        "include/perfetto/protozero/proto_decoder.h",
        "include/perfetto/protozero/proto_utils.h",
        "include/perfetto/protozero/scattered_heap_buffer.h",
        "include/perfetto/protozero/scattered_stream_null_delegate.h",
        "include/perfetto/protozero/scattered_stream_writer.h",
        "include/perfetto/trace_processor/basic_types.h",
        "include/perfetto/trace_processor/trace_processor.h",
        "include/perfetto/traced/sys_stats_counters.h",
        "src/base/event.cc",
        "src/base/file_utils.cc",
        "src/base/metatrace.cc",
        "src/base/paged_memory.cc",
        "src/base/pipe.cc",
        "src/base/string_splitter.cc",
        "src/base/string_utils.cc",
        "src/base/string_view.cc",
        "src/base/temp_file.cc",
        "src/base/thread_checker.cc",
        "src/base/thread_task_runner.cc",
        "src/base/time.cc",
        "src/base/unix_task_runner.cc",
        "src/base/virtual_destructors.cc",
        "src/base/watchdog_posix.cc",
        "src/protozero/message.cc",
        "src/protozero/message_handle.cc",
        "src/protozero/proto_decoder.cc",
        "src/protozero/scattered_heap_buffer.cc",
        "src/protozero/scattered_stream_null_delegate.cc",
        "src/protozero/scattered_stream_writer.cc",
        "src/trace_processor/android_logs_table.cc",
        "src/trace_processor/android_logs_table.h",
        "src/trace_processor/args_table.cc",
        "src/trace_processor/args_table.h",
        "src/trace_processor/args_tracker.cc",
        "src/trace_processor/args_tracker.h",
        "src/trace_processor/chunked_trace_reader.h",
        "src/trace_processor/clock_tracker.cc",
        "src/trace_processor/clock_tracker.h",
        "src/trace_processor/counter_definitions_table.cc",
        "src/trace_processor/counter_definitions_table.h",
        "src/trace_processor/counter_values_table.cc",
        "src/trace_processor/counter_values_table.h",
        "src/trace_processor/event_tracker.cc",
        "src/trace_processor/event_tracker.h",
        "src/trace_processor/filtered_row_index.cc",
        "src/trace_processor/filtered_row_index.h",
        "src/trace_processor/ftrace_descriptors.cc",
        "src/trace_processor/ftrace_descriptors.h",
        "src/trace_processor/ftrace_utils.cc",
        "src/trace_processor/ftrace_utils.h",
        "src/trace_processor/instants_table.cc",
        "src/trace_processor/instants_table.h",
        "src/trace_processor/json_trace_parser.cc",
        "src/trace_processor/json_trace_parser.h",
        "src/trace_processor/null_term_string_view.h",
        "src/trace_processor/process_table.cc",
        "src/trace_processor/process_table.h",
        "src/trace_processor/process_tracker.cc",
        "src/trace_processor/process_tracker.h",
        "src/trace_processor/proto_trace_parser.cc",
        "src/trace_processor/proto_trace_parser.h",
        "src/trace_processor/proto_trace_tokenizer.cc",
        "src/trace_processor/proto_trace_tokenizer.h",
        "src/trace_processor/query_constraints.cc",
        "src/trace_processor/query_constraints.h",
        "src/trace_processor/raw_table.cc",
        "src/trace_processor/raw_table.h",
        "src/trace_processor/row_iterators.cc",
        "src/trace_processor/row_iterators.h",
        "src/trace_processor/sched_slice_table.cc",
        "src/trace_processor/sched_slice_table.h",
        "src/trace_processor/scoped_db.h",
        "src/trace_processor/slice_table.cc",
        "src/trace_processor/slice_table.h",
        "src/trace_processor/slice_tracker.cc",
        "src/trace_processor/slice_tracker.h",
        "src/trace_processor/span_join_operator_table.cc",
        "src/trace_processor/span_join_operator_table.h",
        "src/trace_processor/sql_stats_table.cc",
        "src/trace_processor/sql_stats_table.h",
        "src/trace_processor/sqlite3_str_split.cc",
        "src/trace_processor/sqlite3_str_split.h",
        "src/trace_processor/sqlite_utils.h",
        "src/trace_processor/stats.h",
        "src/trace_processor/stats_table.cc",
        "src/trace_processor/stats_table.h",
        "src/trace_processor/storage_columns.cc",
        "src/trace_processor/storage_columns.h",
        "src/trace_processor/storage_schema.cc",
        "src/trace_processor/storage_schema.h",
        "src/trace_processor/storage_table.cc",
        "src/trace_processor/storage_table.h",
        "src/trace_processor/string_pool.cc",
        "src/trace_processor/string_pool.h",
        "src/trace_processor/string_table.cc",
        "src/trace_processor/string_table.h",
        "src/trace_processor/syscall_tracker.cc",
        "src/trace_processor/syscall_tracker.h",
        "src/trace_processor/table.cc",
        "src/trace_processor/table.h",
        "src/trace_processor/thread_table.cc",
        "src/trace_processor/thread_table.h",
        "src/trace_processor/trace_blob_view.h",
        "src/trace_processor/trace_processor.cc",
        "src/trace_processor/trace_processor_context.cc",
        "src/trace_processor/trace_processor_context.h",
        "src/trace_processor/trace_processor_impl.cc",
        "src/trace_processor/trace_processor_impl.h",
        "src/trace_processor/trace_processor_shell.cc",
        "src/trace_processor/trace_sorter.cc",
        "src/trace_processor/trace_sorter.h",
        "src/trace_processor/trace_storage.cc",
        "src/trace_processor/trace_storage.h",
        "src/trace_processor/virtual_destructors.cc",
        "src/trace_processor/window_operator_table.cc",
        "src/trace_processor/window_operator_table.h",
    ],
    deps = [
        "//third_party/perfetto/google:gtest_prod",
        "//third_party/perfetto/google:jsoncpp",
        "//third_party/perfetto/google:linenoise",
        "//third_party/perfetto/google:perfetto_version",
        "//third_party/perfetto/protos:android_zero_cc_proto",
        "//third_party/perfetto/protos:chrome_zero_cc_proto",
        "//third_party/perfetto/protos:common_cc_proto",
        "//third_party/perfetto/protos:common_zero_cc_proto",
        "//third_party/perfetto/protos:config_zero_cc_proto",
        "//third_party/perfetto/protos:filesystem_zero_cc_proto",
        "//third_party/perfetto/protos:ftrace_zero_cc_proto",
        "//third_party/perfetto/protos:interned_data_zero_cc_proto",
        "//third_party/perfetto/protos:power_zero_cc_proto",
        "//third_party/perfetto/protos:profiling_zero_cc_proto",
        "//third_party/perfetto/protos:ps_zero_cc_proto",
        "//third_party/perfetto/protos:sys_stats_zero_cc_proto",
        "//third_party/perfetto/protos:trace_processor_cc_proto",
        "//third_party/perfetto/protos:trace_zero_cc_proto",
        "//third_party/perfetto/protos:track_event_zero_cc_proto",
        "//third_party/sqlite",
        "//third_party/sqlite:sqlite_ext_percentile",
    ],
)

# GN target:
# //tools/trace_to_text:trace_to_text_host(//gn/standalone/toolchain:gcc_like_host)
cc_binary(
    name = "tools_trace_to_text_trace_to_text_host",
    srcs = [
        "include/perfetto/base/build_config.h",
        "include/perfetto/base/circular_queue.h",
        "include/perfetto/base/container_annotations.h",
        "include/perfetto/base/event.h",
        "include/perfetto/base/export.h",
        "include/perfetto/base/file_utils.h",
        "include/perfetto/base/gtest_prod_util.h",
        "include/perfetto/base/hash.h",
        "include/perfetto/base/logging.h",
        "include/perfetto/base/metatrace.h",
        "include/perfetto/base/optional.h",
        "include/perfetto/base/paged_memory.h",
        "include/perfetto/base/pipe.h",
        "include/perfetto/base/scoped_file.h",
        "include/perfetto/base/small_set.h",
        "include/perfetto/base/string_splitter.h",
        "include/perfetto/base/string_utils.h",
        "include/perfetto/base/string_view.h",
        "include/perfetto/base/string_writer.h",
        "include/perfetto/base/task_runner.h",
        "include/perfetto/base/temp_file.h",
        "include/perfetto/base/thread_checker.h",
        "include/perfetto/base/thread_task_runner.h",
        "include/perfetto/base/thread_utils.h",
        "include/perfetto/base/time.h",
        "include/perfetto/base/unix_socket.h",
        "include/perfetto/base/unix_task_runner.h",
        "include/perfetto/base/utils.h",
        "include/perfetto/base/watchdog.h",
        "include/perfetto/base/watchdog_noop.h",
        "include/perfetto/base/watchdog_posix.h",
        "include/perfetto/base/weak_ptr.h",
        "include/perfetto/protozero/contiguous_memory_range.h",
        "include/perfetto/protozero/field.h",
        "include/perfetto/protozero/message.h",
        "include/perfetto/protozero/message_handle.h",
        "include/perfetto/protozero/proto_decoder.h",
        "include/perfetto/protozero/proto_utils.h",
        "include/perfetto/protozero/scattered_heap_buffer.h",
        "include/perfetto/protozero/scattered_stream_null_delegate.h",
        "include/perfetto/protozero/scattered_stream_writer.h",
        "include/perfetto/trace_processor/basic_types.h",
        "include/perfetto/trace_processor/trace_processor.h",
        "include/perfetto/traced/sys_stats_counters.h",
        "src/base/event.cc",
        "src/base/file_utils.cc",
        "src/base/metatrace.cc",
        "src/base/paged_memory.cc",
        "src/base/pipe.cc",
        "src/base/string_splitter.cc",
        "src/base/string_utils.cc",
        "src/base/string_view.cc",
        "src/base/temp_file.cc",
        "src/base/thread_checker.cc",
        "src/base/thread_task_runner.cc",
        "src/base/time.cc",
        "src/base/unix_task_runner.cc",
        "src/base/virtual_destructors.cc",
        "src/base/watchdog_posix.cc",
        "src/protozero/message.cc",
        "src/protozero/message_handle.cc",
        "src/protozero/proto_decoder.cc",
        "src/protozero/scattered_heap_buffer.cc",
        "src/protozero/scattered_stream_null_delegate.cc",
        "src/protozero/scattered_stream_writer.cc",
        "src/trace_processor/android_logs_table.cc",
        "src/trace_processor/android_logs_table.h",
        "src/trace_processor/args_table.cc",
        "src/trace_processor/args_table.h",
        "src/trace_processor/args_tracker.cc",
        "src/trace_processor/args_tracker.h",
        "src/trace_processor/chunked_trace_reader.h",
        "src/trace_processor/clock_tracker.cc",
        "src/trace_processor/clock_tracker.h",
        "src/trace_processor/counter_definitions_table.cc",
        "src/trace_processor/counter_definitions_table.h",
        "src/trace_processor/counter_values_table.cc",
        "src/trace_processor/counter_values_table.h",
        "src/trace_processor/event_tracker.cc",
        "src/trace_processor/event_tracker.h",
        "src/trace_processor/filtered_row_index.cc",
        "src/trace_processor/filtered_row_index.h",
        "src/trace_processor/ftrace_descriptors.cc",
        "src/trace_processor/ftrace_descriptors.h",
        "src/trace_processor/ftrace_utils.cc",
        "src/trace_processor/ftrace_utils.h",
        "src/trace_processor/instants_table.cc",
        "src/trace_processor/instants_table.h",
        "src/trace_processor/json_trace_parser.cc",
        "src/trace_processor/json_trace_parser.h",
        "src/trace_processor/null_term_string_view.h",
        "src/trace_processor/process_table.cc",
        "src/trace_processor/process_table.h",
        "src/trace_processor/process_tracker.cc",
        "src/trace_processor/process_tracker.h",
        "src/trace_processor/proto_trace_parser.cc",
        "src/trace_processor/proto_trace_parser.h",
        "src/trace_processor/proto_trace_tokenizer.cc",
        "src/trace_processor/proto_trace_tokenizer.h",
        "src/trace_processor/query_constraints.cc",
        "src/trace_processor/query_constraints.h",
        "src/trace_processor/raw_table.cc",
        "src/trace_processor/raw_table.h",
        "src/trace_processor/row_iterators.cc",
        "src/trace_processor/row_iterators.h",
        "src/trace_processor/sched_slice_table.cc",
        "src/trace_processor/sched_slice_table.h",
        "src/trace_processor/scoped_db.h",
        "src/trace_processor/slice_table.cc",
        "src/trace_processor/slice_table.h",
        "src/trace_processor/slice_tracker.cc",
        "src/trace_processor/slice_tracker.h",
        "src/trace_processor/span_join_operator_table.cc",
        "src/trace_processor/span_join_operator_table.h",
        "src/trace_processor/sql_stats_table.cc",
        "src/trace_processor/sql_stats_table.h",
        "src/trace_processor/sqlite3_str_split.cc",
        "src/trace_processor/sqlite3_str_split.h",
        "src/trace_processor/sqlite_utils.h",
        "src/trace_processor/stats.h",
        "src/trace_processor/stats_table.cc",
        "src/trace_processor/stats_table.h",
        "src/trace_processor/storage_columns.cc",
        "src/trace_processor/storage_columns.h",
        "src/trace_processor/storage_schema.cc",
        "src/trace_processor/storage_schema.h",
        "src/trace_processor/storage_table.cc",
        "src/trace_processor/storage_table.h",
        "src/trace_processor/string_pool.cc",
        "src/trace_processor/string_pool.h",
        "src/trace_processor/string_table.cc",
        "src/trace_processor/string_table.h",
        "src/trace_processor/syscall_tracker.cc",
        "src/trace_processor/syscall_tracker.h",
        "src/trace_processor/table.cc",
        "src/trace_processor/table.h",
        "src/trace_processor/thread_table.cc",
        "src/trace_processor/thread_table.h",
        "src/trace_processor/trace_blob_view.h",
        "src/trace_processor/trace_processor.cc",
        "src/trace_processor/trace_processor_context.cc",
        "src/trace_processor/trace_processor_context.h",
        "src/trace_processor/trace_processor_impl.cc",
        "src/trace_processor/trace_processor_impl.h",
        "src/trace_processor/trace_sorter.cc",
        "src/trace_processor/trace_sorter.h",
        "src/trace_processor/trace_storage.cc",
        "src/trace_processor/trace_storage.h",
        "src/trace_processor/virtual_destructors.cc",
        "src/trace_processor/window_operator_table.cc",
        "src/trace_processor/window_operator_table.h",
        "tools/trace_to_text/main.cc",
        "tools/trace_to_text/proto_full_utils.cc",
        "tools/trace_to_text/proto_full_utils.h",
        "tools/trace_to_text/trace_to_profile.cc",
        "tools/trace_to_text/trace_to_profile.h",
        "tools/trace_to_text/trace_to_systrace.cc",
        "tools/trace_to_text/trace_to_systrace.h",
        "tools/trace_to_text/trace_to_text.cc",
        "tools/trace_to_text/trace_to_text.h",
        "tools/trace_to_text/utils.cc",
        "tools/trace_to_text/utils.h",
    ],
    deps = [
        "//third_party/perfetto/google:gtest_prod",
        "//third_party/perfetto/google:jsoncpp",
        "//third_party/perfetto/google:perfetto_version",
        "//third_party/perfetto/protos:android_cc_proto",
        "//third_party/perfetto/protos:android_zero_cc_proto",
        "//third_party/perfetto/protos:chrome_cc_proto",
        "//third_party/perfetto/protos:chrome_zero_cc_proto",
        "//third_party/perfetto/protos:common_cc_proto",
        "//third_party/perfetto/protos:common_zero_cc_proto",
        "//third_party/perfetto/protos:config_cc_proto",
        "//third_party/perfetto/protos:config_zero_cc_proto",
        "//third_party/perfetto/protos:filesystem_cc_proto",
        "//third_party/perfetto/protos:filesystem_zero_cc_proto",
        "//third_party/perfetto/protos:ftrace_cc_proto",
        "//third_party/perfetto/protos:ftrace_zero_cc_proto",
        "//third_party/perfetto/protos:interned_data_cc_proto",
        "//third_party/perfetto/protos:interned_data_zero_cc_proto",
        "//third_party/perfetto/protos:power_cc_proto",
        "//third_party/perfetto/protos:power_zero_cc_proto",
        "//third_party/perfetto/protos:pprof_cc_proto",
        "//third_party/perfetto/protos:profiling_cc_proto",
        "//third_party/perfetto/protos:profiling_zero_cc_proto",
        "//third_party/perfetto/protos:ps_cc_proto",
        "//third_party/perfetto/protos:ps_zero_cc_proto",
        "//third_party/perfetto/protos:sys_stats_cc_proto",
        "//third_party/perfetto/protos:sys_stats_zero_cc_proto",
        "//third_party/perfetto/protos:trace_cc_proto",
        "//third_party/perfetto/protos:trace_minimal_cc_proto",
        "//third_party/perfetto/protos:trace_processor_cc_proto",
        "//third_party/perfetto/protos:trace_zero_cc_proto",
        "//third_party/perfetto/protos:track_event_cc_proto",
        "//third_party/perfetto/protos:track_event_zero_cc_proto",
        "//third_party/protobuf",
        "//third_party/protobuf:libprotoc",
        "//third_party/sqlite",
        "//third_party/sqlite:sqlite_ext_percentile",
    ],
)

alias(
    name = "trace_processor",
    actual = ":src_trace_processor_trace_processor",
)

genrule(
    name = "trace_processor_shell_rule",
    srcs = [
        ":src_trace_processor_trace_processor_shell_host",
    ],
    outs = [
        "trace_processor_shell",
    ],
    cmd = "cp $(location :src_trace_processor_trace_processor_shell_host) $@",
    executable = 1,
)

gensignature(
    name = "trace_processor_shell_sig",
    srcs = [
        ":trace_processor_shell",
    ],
)

genrule(
    name = "trace_to_text_rule",
    srcs = [
        ":tools_trace_to_text_trace_to_text_host",
    ],
    outs = [
        "trace_to_text",
    ],
    cmd = "cp $(location :tools_trace_to_text_trace_to_text_host) $@",
    executable = 1,
)

gensignature(
    name = "trace_to_text_sig",
    srcs = [
        ":trace_to_text",
    ],
)
