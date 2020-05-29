/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SRC_TRACE_PROCESSOR_TABLES_PROFILER_TABLES_H_
#define SRC_TRACE_PROCESSOR_TABLES_PROFILER_TABLES_H_

#include "src/trace_processor/tables/macros.h"
#include "src/trace_processor/tables/track_tables.h"

namespace perfetto {
namespace trace_processor {
namespace tables {

// The profiler smaps contains the memory stats for virtual memory ranges
// captured by the [heap profiler](/docs/data-sources/native-heap-profiler.md).
// @param upid The UniquePID of the process {@joinable process.upid}.
// @param ts   Timestamp of the snapshot. Multiple rows will have the same
//             timestamp.
// @param path The mmaped file, as per /proc/pid/smaps.
// @param size_kb Total size of the mapping.
// @param private_dirty_kb KB of this mapping that are private dirty  RSS.
// @param swap_kb KB of this mapping that are in swap.
// @tablegroup Callstack profilers
#define PERFETTO_TP_PROFILER_SMAPS_DEF(NAME, PARENT, C) \
  NAME(ProfilerSmapsTable, "profiler_smaps")            \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                     \
  C(uint32_t, upid)                                     \
  C(int64_t, ts)                                        \
  C(StringPool::Id, path)                               \
  C(int64_t, size_kb)                                   \
  C(int64_t, private_dirty_kb)                          \
  C(int64_t, swap_kb)

PERFETTO_TP_TABLE(PERFETTO_TP_PROFILER_SMAPS_DEF);

// Metadata about packages installed on the system.
// This is generated by the packages_list data-source.
// @param package_name name of the package, e.g. com.google.android.gm.
// @param uid UID processes of this package run as.
// @param debuggable bool whether this app is debuggable.
// @param profileable_from_shell bool whether this app is profileable.
// @param version_code versionCode from the APK.
#define PERFETTO_TP_PACKAGES_LIST_DEF(NAME, PARENT, C) \
  NAME(PackageListTable, "package_list")               \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                    \
  C(StringPool::Id, package_name)                      \
  C(int64_t, uid)                                      \
  C(int32_t, debuggable)                               \
  C(int32_t, profileable_from_shell)                   \
  C(int64_t, version_code)

PERFETTO_TP_TABLE(PERFETTO_TP_PACKAGES_LIST_DEF);

// A mapping (binary / library) in a process.
// This is generated by the stack profilers: heapprofd and traced_perf.
// @param build_id hex-encoded Build ID of the binary / library.
// @param start start of the mapping in the process' address space.
// @param end end of the mapping in the process' address space.
// @param name filename of the binary / library {@joinable profiler_smaps.path}.
// @tablegroup Callstack profilers
#define PERFETTO_TP_STACK_PROFILE_MAPPING_DEF(NAME, PARENT, C) \
  NAME(StackProfileMappingTable, "stack_profile_mapping")      \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                            \
  C(StringPool::Id, build_id)                                  \
  C(int64_t, exact_offset)                                     \
  C(int64_t, start_offset)                                     \
  C(int64_t, start)                                            \
  C(int64_t, end)                                              \
  C(int64_t, load_bias)                                        \
  C(StringPool::Id, name)

PERFETTO_TP_TABLE(PERFETTO_TP_STACK_PROFILE_MAPPING_DEF);

// A frame on the callstack. This is a location in a program.
// This is generated by the stack profilers: heapprofd and traced_perf.
// @param name name of the function this location is in.
// @param mapping the mapping (library / binary) this location is in.
// @param rel_pc the program counter relative to the start of the mapping.
// @param symbol_set_id if the profile was offline symbolized, the offline
//        symbol information of this frame.
//        {@joinable stack_profile_symbol.symbol_set_id}
// @tablegroup Callstack profilers
#define PERFETTO_TP_STACK_PROFILE_FRAME_DEF(NAME, PARENT, C) \
  NAME(StackProfileFrameTable, "stack_profile_frame")        \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                          \
  C(StringPool::Id, name)                                    \
  C(StackProfileMappingTable::Id, mapping)                   \
  C(int64_t, rel_pc)                                         \
  C(base::Optional<uint32_t>, symbol_set_id)

PERFETTO_TP_TABLE(PERFETTO_TP_STACK_PROFILE_FRAME_DEF);

// A callsite. This is a list of frames that were on the stack.
// This is generated by the stack profilers: heapprofd and traced_perf.
// @param depth distance from the bottom-most frame of the callstack.
// @param parent_id parent frame on the callstack. NULL for the bottom-most.
// @param frame_id frame at this position in the callstack.
// @tablegroup Callstack profilers
#define PERFETTO_TP_STACK_PROFILE_CALLSITE_DEF(NAME, PARENT, C) \
  NAME(StackProfileCallsiteTable, "stack_profile_callsite")     \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                             \
  C(uint32_t, depth)                                            \
  C(base::Optional<StackProfileCallsiteTable::Id>, parent_id)   \
  C(StackProfileFrameTable::Id, frame_id)

PERFETTO_TP_TABLE(PERFETTO_TP_STACK_PROFILE_CALLSITE_DEF);

// This is generated by traced_perf.
// @param ts timestamp this sample was taken at.
// @param utid thread that was active when the sample was taken.
// @param callsite_id callstack in active thread at time of sample.
// @tablegroup Callstack profilers
#define PERFETTO_TP_CPU_PROFILE_STACK_SAMPLE_DEF(NAME, PARENT, C) \
  NAME(CpuProfileStackSampleTable, "cpu_profile_stack_sample")    \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                               \
  C(int64_t, ts, Column::Flag::kSorted)                           \
  C(StackProfileCallsiteTable::Id, callsite_id)                   \
  C(uint32_t, utid)                                               \
  C(int32_t, process_priority)

PERFETTO_TP_TABLE(PERFETTO_TP_CPU_PROFILE_STACK_SAMPLE_DEF);

// Symbolization data for a frame. Rows with them same symbol_set_id describe
// one frame, with the bottom-most inlined frame having id == symbol_set_id.
//
// For instance, if the function foo has an inlined call to the function bar,
// which has an inlined call to baz, the stack_profile_symbol table would look
// like this.
//
// ```
// |id|symbol_set_id|name         |source_file|line_number|
// |--|-------------|-------------|-----------|-----------|
// |1 |      1      |foo          |foo.cc     | 60        |
// |2 |      1      |bar          |foo.cc     | 30        |
// |3 |      1      |baz          |foo.cc     | 36        |
// ```
// @param name name of the function.
// @param source_file name of the source file containing the function.
// @param line_number line number of the frame in the source file. This is the
// exact line for the corresponding program counter, not the beginning of the
// function.
// @tablegroup Callstack profilers
#define PERFETTO_TP_SYMBOL_DEF(NAME, PARENT, C) \
  NAME(SymbolTable, "stack_profile_symbol")     \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)             \
  C(uint32_t, symbol_set_id)                    \
  C(StringPool::Id, name)                       \
  C(StringPool::Id, source_file)                \
  C(uint32_t, line_number)

PERFETTO_TP_TABLE(PERFETTO_TP_SYMBOL_DEF);

// Allocations that happened at a callsite.
// This is generated by heapprofd.
// @param ts the timestamp the allocations happened at. heapprofd batches
// allocations and frees, and all data from a dump will have the same
// timestamp.
// @param upid the UniquePID of the allocating process.
//        {@joinable process.upid}
// @param callsite_id the callsite the allocation happened at.
// @param count if positive: number of allocations that happened at this
// callsite. if negative: number of allocations that happened at this callsite
// that were freed.
// @param size if positive: size of allocations that happened at this
// callsite. if negative: size of allocations that happened at this callsite
// that were freed.
// @tablegroup Callstack profilers
#define PERFETTO_TP_HEAP_PROFILE_ALLOCATION_DEF(NAME, PARENT, C) \
  NAME(HeapProfileAllocationTable, "heap_profile_allocation")    \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                              \
  C(int64_t, ts, Column::Flag::kSorted)                          \
  C(uint32_t, upid)                                              \
  C(StackProfileCallsiteTable::Id, callsite_id)                  \
  C(int64_t, count)                                              \
  C(int64_t, size)

PERFETTO_TP_TABLE(PERFETTO_TP_HEAP_PROFILE_ALLOCATION_DEF);

// Table used to render flamegraphs. This gives cumulative sizes of nodes in
// the flamegraph.
//
// WARNING: This is experimental and the API is subject to change.
// @tablegroup Callstack profilers
#define PERFETTO_TP_EXPERIMENTAL_FLAMEGRAPH_NODES(NAME, PARENT, C)        \
  NAME(ExperimentalFlamegraphNodesTable, "experimental_flamegraph_nodes") \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                                       \
  C(int64_t, ts, Column::Flag::kSorted | Column::Flag::kHidden)           \
  C(uint32_t, upid, Column::Flag::kHidden)                                \
  C(StringPool::Id, profile_type, Column::Flag::kHidden)                  \
  C(StringPool::Id, focus_str, Column::Flag::kHidden)                     \
  C(uint32_t, depth)                                                      \
  C(StringPool::Id, name)                                                 \
  C(StringPool::Id, map_name)                                             \
  C(int64_t, count)                                                       \
  C(int64_t, cumulative_count)                                            \
  C(int64_t, size)                                                        \
  C(int64_t, cumulative_size)                                             \
  C(int64_t, alloc_count)                                                 \
  C(int64_t, cumulative_alloc_count)                                      \
  C(int64_t, alloc_size)                                                  \
  C(int64_t, cumulative_alloc_size)                                       \
  C(base::Optional<ExperimentalFlamegraphNodesTable::Id>, parent_id)

PERFETTO_TP_TABLE(PERFETTO_TP_EXPERIMENTAL_FLAMEGRAPH_NODES);

// @param name (potentially obfuscated) name of the class.
// @param deobfuscated_name if class name was obfuscated and deobfuscation map
// for it provided, the deobfuscated name.
// @param location the APK / Dex / JAR file the class is contained in.
// @tablegroup ART Heap Profiler
#define PERFETTO_TP_HEAP_GRAPH_CLASS_DEF(NAME, PARENT, C) \
  NAME(HeapGraphClassTable, "heap_graph_class")           \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                       \
  C(StringPool::Id, name)                                 \
  C(base::Optional<StringPool::Id>, deobfuscated_name)    \
  C(base::Optional<StringPool::Id>, location)

PERFETTO_TP_TABLE(PERFETTO_TP_HEAP_GRAPH_CLASS_DEF);

// The objects on the Dalvik heap.
//
// All rows with the same (upid, graph_sample_ts) are one dump.
// @param upid UniquePid of the target {@joinable process.upid}.
// @param graph_sample_ts timestamp this dump was taken at.
// @param self_size size this object uses on the Java Heap.
// @param reference_set_id join key with heap_graph_reference containing all
//        objects referred in this object's fields.
//        {@joinable heap_graph_reference.reference_set_id}
// @param reachable bool whether this object is reachable from a GC root. If
// false, this object is uncollected garbage.
// @param type_id class this object is an instance of.
// @param root_type if not NULL, this object is a GC root.
// @tablegroup ART Heap Profiler
#define PERFETTO_TP_HEAP_GRAPH_OBJECT_DEF(NAME, PARENT, C) \
  NAME(HeapGraphObjectTable, "heap_graph_object")          \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                        \
  C(uint32_t, upid)                                        \
  C(int64_t, graph_sample_ts)                              \
  C(int64_t, self_size)                                    \
  C(base::Optional<uint32_t>, reference_set_id)            \
  C(int32_t, reachable)                                    \
  C(HeapGraphClassTable::Id, type_id)                      \
  C(base::Optional<StringPool::Id>, root_type)             \
  C(int32_t, root_distance, Column::Flag::kHidden)

PERFETTO_TP_TABLE(PERFETTO_TP_HEAP_GRAPH_OBJECT_DEF);

// Many-to-many mapping between heap_graph_object.
//
// This associates the object with given reference_set_id with the objects
// that are referred to by its fields.
// @param reference_set_id join key to heap_graph_object.
// @param owner_id id of object that has this reference_set_id.
// @param owned_id id of object that is referred to.
// @param field_name the field that refers to the object. E.g. Foo.name.
// @param field_type_name the static type of the field. E.g. java.lang.String.
// @param deobfuscated_field_name if field_name was obfuscated and a
// deobfuscation mapping was provided for it, the deobfuscated name.
// @tablegroup ART Heap Profiler
#define PERFETTO_TP_HEAP_GRAPH_REFERENCE_DEF(NAME, PARENT, C) \
  NAME(HeapGraphReferenceTable, "heap_graph_reference")       \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                           \
  C(uint32_t, reference_set_id, Column::Flag::kSorted)        \
  C(HeapGraphObjectTable::Id, owner_id)                       \
  C(HeapGraphObjectTable::Id, owned_id)                       \
  C(StringPool::Id, field_name)                               \
  C(StringPool::Id, field_type_name)                          \
  C(base::Optional<StringPool::Id>, deobfuscated_field_name)

PERFETTO_TP_TABLE(PERFETTO_TP_HEAP_GRAPH_REFERENCE_DEF);

// @param arg_set_id {@joinable args.arg_set_id}
#define PERFETTO_TP_VULKAN_MEMORY_ALLOCATIONS_DEF(NAME, PARENT, C) \
  NAME(VulkanMemoryAllocationsTable, "vulkan_memory_allocations")  \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                                \
  C(StringPool::Id, source)                                        \
  C(StringPool::Id, operation)                                     \
  C(int64_t, timestamp)                                            \
  C(base::Optional<uint32_t>, upid)                                \
  C(base::Optional<int64_t>, device)                               \
  C(base::Optional<int64_t>, device_memory)                        \
  C(base::Optional<uint32_t>, memory_type)                         \
  C(base::Optional<uint32_t>, heap)                                \
  C(base::Optional<StringPool::Id>, function_name)                 \
  C(base::Optional<int64_t>, object_handle)                        \
  C(base::Optional<int64_t>, memory_address)                       \
  C(base::Optional<int64_t>, memory_size)                          \
  C(StringPool::Id, scope)                                         \
  C(base::Optional<uint32_t>, arg_set_id)

PERFETTO_TP_TABLE(PERFETTO_TP_VULKAN_MEMORY_ALLOCATIONS_DEF);

#define PERFETTO_TP_GPU_COUNTER_GROUP_DEF(NAME, PARENT, C) \
  NAME(GpuCounterGroupTable, "gpu_counter_group")          \
  PERFETTO_TP_ROOT_TABLE(PARENT, C)                        \
  C(int32_t, group_id)                                     \
  C(TrackTable::Id, track_id)

PERFETTO_TP_TABLE(PERFETTO_TP_GPU_COUNTER_GROUP_DEF);

}  // namespace tables
}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_TABLES_PROFILER_TABLES_H_
