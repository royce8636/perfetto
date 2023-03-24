# Copyright (C) 2023 The Android Open Source Project
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
"""Contains tables for relevant for TODO."""

from python.generators.trace_processor_table.public import Column as C
from python.generators.trace_processor_table.public import ColumnFlag
from python.generators.trace_processor_table.public import CppInt32
from python.generators.trace_processor_table.public import CppInt64
from python.generators.trace_processor_table.public import CppOptional
from python.generators.trace_processor_table.public import CppSelfTableId
from python.generators.trace_processor_table.public import CppString
from python.generators.trace_processor_table.public import Table
from python.generators.trace_processor_table.public import TableDoc
from python.generators.trace_processor_table.public import CppTableId
from python.generators.trace_processor_table.public import CppUint32

from src.trace_processor.tables.track_tables import TRACK_TABLE

PROFILER_SMAPS_TABLE = Table(
    class_name='ProfilerSmapsTable',
    sql_name='profiler_smaps',
    columns=[
        C('upid', CppUint32()),
        C('ts', CppInt64()),
        C('path', CppString()),
        C('size_kb', CppInt64()),
        C('private_dirty_kb', CppInt64()),
        C('swap_kb', CppInt64()),
        C('file_name', CppString()),
        C('start_address', CppInt64()),
        C('module_timestamp', CppInt64()),
        C('module_debugid', CppString()),
        C('module_debug_path', CppString()),
        C('protection_flags', CppInt64()),
        C('private_clean_resident_kb', CppInt64()),
        C('shared_dirty_resident_kb', CppInt64()),
        C('shared_clean_resident_kb', CppInt64()),
        C('locked_kb', CppInt64()),
        C('proportional_resident_kb', CppInt64()),
    ],
    tabledoc=TableDoc(
        doc='''
          The profiler smaps contains the memory stats for virtual memory ranges
captured by the [heap profiler](/docs/data-sources/native-heap-profiler.md).
        ''',
        group='Callstack profilers',
        columns={
            'upid':
                '''The UniquePID of the process.''',
            'ts':
                '''Timestamp of the snapshot. Multiple rows will have the same
timestamp.''',
            'path':
                '''The mmaped file, as per /proc/pid/smaps.''',
            'size_kb':
                '''Total size of the mapping.''',
            'private_dirty_kb':
                '''KB of this mapping that are private dirty  RSS.''',
            'swap_kb':
                '''KB of this mapping that are in swap.''',
            'file_name':
                '''''',
            'start_address':
                '''''',
            'module_timestamp':
                '''''',
            'module_debugid':
                '''''',
            'module_debug_path':
                '''''',
            'protection_flags':
                '''''',
            'private_clean_resident_kb':
                '''''',
            'shared_dirty_resident_kb':
                '''''',
            'shared_clean_resident_kb':
                '''''',
            'locked_kb':
                '''''',
            'proportional_resident_kb':
                ''''''
        }))

PACKAGE_LIST_TABLE = Table(
    class_name='PackageListTable',
    sql_name='package_list',
    columns=[
        C('package_name', CppString()),
        C('uid', CppInt64()),
        C('debuggable', CppInt32()),
        C('profileable_from_shell', CppInt32()),
        C('version_code', CppInt64()),
    ],
    tabledoc=TableDoc(
        doc='''
          Metadata about packages installed on the system.
This is generated by the packages_list data-source.
        ''',
        group='Misc',
        columns={
            'package_name':
                '''name of the package, e.g. com.google.android.gm.''',
            'uid':
                '''UID processes of this package run as.''',
            'debuggable':
                '''bool whether this app is debuggable.''',
            'profileable_from_shell':
                '''bool whether this app is profileable.''',
            'version_code':
                '''versionCode from the APK.'''
        }))

STACK_PROFILE_MAPPING_TABLE = Table(
    class_name='StackProfileMappingTable',
    sql_name='stack_profile_mapping',
    columns=[
        C('build_id', CppString()),
        C('exact_offset', CppInt64()),
        C('start_offset', CppInt64()),
        C('start', CppInt64()),
        C('end', CppInt64()),
        C('load_bias', CppInt64()),
        C('name', CppString()),
    ],
    tabledoc=TableDoc(
        doc='''
          A mapping (binary / library) in a process.
This is generated by the stack profilers: heapprofd and traced_perf.
        ''',
        group='Callstack profilers',
        columns={
            'build_id': '''hex-encoded Build ID of the binary / library.''',
            'start': '''start of the mapping in the process' address space.''',
            'end': '''end of the mapping in the process' address space.''',
            'name': '''filename of the binary / library.''',
            'exact_offset': '''''',
            'start_offset': '''''',
            'load_bias': ''''''
        }))

STACK_PROFILE_FRAME_TABLE = Table(
    class_name='StackProfileFrameTable',
    sql_name='stack_profile_frame',
    columns=[
        C('name', CppString()),
        C('mapping', CppTableId(STACK_PROFILE_MAPPING_TABLE)),
        C('rel_pc', CppInt64()),
        C('symbol_set_id', CppOptional(CppUint32())),
        C('deobfuscated_name', CppOptional(CppString())),
    ],
    tabledoc=TableDoc(
        doc='''
          A frame on the callstack. This is a location in a program.
This is generated by the stack profilers: heapprofd and traced_perf.
        ''',
        group='Callstack profilers',
        columns={
            'name':
                '''name of the function this location is in.''',
            'mapping':
                '''the mapping (library / binary) this location is in.''',
            'rel_pc':
                '''the program counter relative to the start of the mapping.''',
            'symbol_set_id':
                '''if the profile was offline symbolized, the offline
symbol information of this frame.''',
            'deobfuscated_name':
                ''''''
        }))

STACK_PROFILE_CALLSITE_TABLE = Table(
    class_name='StackProfileCallsiteTable',
    sql_name='stack_profile_callsite',
    columns=[
        C('depth', CppUint32()),
        C('parent_id', CppOptional(CppSelfTableId())),
        C('frame_id', CppTableId(STACK_PROFILE_FRAME_TABLE)),
    ],
    tabledoc=TableDoc(
        doc='''
          A callsite. This is a list of frames that were on the stack.
This is generated by the stack profilers: heapprofd and traced_perf.
        ''',
        group='Callstack profilers',
        columns={
            'depth':
                '''distance from the bottom-most frame of the callstack.''',
            'parent_id':
                '''parent frame on the callstack. NULL for the bottom-most.''',
            'frame_id':
                '''frame at this position in the callstack.'''
        }))

STACK_SAMPLE_TABLE = Table(
    class_name='StackSampleTable',
    sql_name='stack_sample',
    columns=[
        C('ts', CppInt64(), flags=ColumnFlag.SORTED),
        C('callsite_id', CppTableId(STACK_PROFILE_CALLSITE_TABLE)),
    ],
    tabledoc=TableDoc(
        doc='''
          Root table for timestamped stack samples.
        ''',
        group='Callstack profilers',
        columns={
            'ts': '''timestamp of the sample.''',
            'callsite_id': '''unwound callstack.'''
        }))

CPU_PROFILE_STACK_SAMPLE_TABLE = Table(
    class_name='CpuProfileStackSampleTable',
    sql_name='cpu_profile_stack_sample',
    columns=[
        C('utid', CppUint32()),
        C('process_priority', CppInt32()),
    ],
    parent=STACK_SAMPLE_TABLE,
    tabledoc=TableDoc(
        doc='''
          Samples from the Chromium stack sampler.
        ''',
        group='Callstack profilers',
        columns={
            'utid': '''thread that was active when the sample was taken.''',
            'process_priority': ''''''
        }))

PERF_SAMPLE_TABLE = Table(
    class_name='PerfSampleTable',
    sql_name='perf_sample',
    columns=[
        C('ts', CppInt64(), flags=ColumnFlag.SORTED),
        C('utid', CppUint32()),
        C('cpu', CppUint32()),
        C('cpu_mode', CppString()),
        C('callsite_id', CppOptional(CppTableId(STACK_PROFILE_CALLSITE_TABLE))),
        C('unwind_error', CppOptional(CppString())),
        C('perf_session_id', CppUint32()),
    ],
    tabledoc=TableDoc(
        doc='''
          Samples from the traced_perf profiler.
        ''',
        group='Callstack profilers',
        columns={
            'ts':
                '''timestamp of the sample.''',
            'utid':
                '''sampled thread..''',
            'cpu':
                '''the core the sampled thread was running on.''',
            'cpu_mode':
                '''execution state (userspace/kernelspace) of the sampled
thread.''',
            'callsite_id':
                '''if set, unwound callstack of the sampled thread.''',
            'unwind_error':
                '''if set, indicates that the unwinding for this sample
encountered an error. Such samples still reference the best-effort
result via the callsite_id (with a synthetic error frame at the point
where unwinding stopped).''',
            'perf_session_id':
                '''distinguishes samples from different profiling
streams (i.e. multiple data sources).'''
        }))

SYMBOL_TABLE = Table(
    class_name='SymbolTable',
    sql_name='stack_profile_symbol',
    columns=[
        C('symbol_set_id',
          CppUint32(),
          flags=ColumnFlag.SORTED | ColumnFlag.SET_ID),
        C('name', CppString()),
        C('source_file', CppString()),
        C('line_number', CppUint32()),
    ],
    tabledoc=TableDoc(
        doc='''
            Symbolization data for a frame. Rows with the same symbol_set_id
            describe one callframe, with the most-inlined symbol having
            id == symbol_set_id.

            For instance, if the function foo has an inlined call to the
            function bar, which has an inlined call to baz, the
            stack_profile_symbol table would look like this.

            ```
            |id|symbol_set_id|name         |source_file|line_number|
            |--|-------------|-------------|-----------|-----------|
            |1 |      1      |baz          |foo.cc     | 36        |
            |2 |      1      |bar          |foo.cc     | 30        |
            |3 |      1      |foo          |foo.cc     | 60        |
            ```
        ''',
        group='Callstack profilers',
        columns={
            'name':
                '''name of the function.''',
            'source_file':
                '''name of the source file containing the function.''',
            'line_number':
                '''
                    line number of the frame in the source file. This is the
                    exact line for the corresponding program counter, not the
                    beginning of the function.
                ''',
            'symbol_set_id':
                ''''''
        }))

HEAP_PROFILE_ALLOCATION_TABLE = Table(
    class_name='HeapProfileAllocationTable',
    sql_name='heap_profile_allocation',
    columns=[
        C('ts', CppInt64()),
        C('upid', CppUint32()),
        C('heap_name', CppString()),
        C('callsite_id', CppTableId(STACK_PROFILE_CALLSITE_TABLE)),
        C('count', CppInt64()),
        C('size', CppInt64()),
    ],
    tabledoc=TableDoc(
        doc='''
          Allocations that happened at a callsite.


NOTE: this table is not sorted by timestamp intentionanlly - see b/193757386
for details.
TODO(b/193757386): readd the sorted flag once this bug is fixed.

This is generated by heapprofd.
        ''',
        group='Callstack profilers',
        columns={
            'ts':
                '''the timestamp the allocations happened at. heapprofd batches
allocations and frees, and all data from a dump will have the same
timestamp.''',
            'upid':
                '''the UniquePID of the allocating process.''',
            'callsite_id':
                '''the callsite the allocation happened at.''',
            'count':
                '''if positive: number of allocations that happened at this
callsite. if negative: number of allocations that happened at this callsite
that were freed.''',
            'size':
                '''if positive: size of allocations that happened at this
callsite. if negative: size of allocations that happened at this callsite
that were freed.''',
            'heap_name':
                ''''''
        }))

EXPERIMENTAL_FLAMEGRAPH_NODES_TABLE = Table(
    class_name='ExperimentalFlamegraphNodesTable',
    sql_name='experimental_flamegraph_nodes',
    columns=[
        C('ts', CppInt64(), flags=ColumnFlag.SORTED | ColumnFlag.HIDDEN),
        C('upid', CppUint32(), flags=ColumnFlag.HIDDEN),
        C('profile_type', CppString(), flags=ColumnFlag.HIDDEN),
        C('focus_str', CppString(), flags=ColumnFlag.HIDDEN),
        C('depth', CppUint32()),
        C('name', CppString()),
        C('map_name', CppString()),
        C('count', CppInt64()),
        C('cumulative_count', CppInt64()),
        C('size', CppInt64()),
        C('cumulative_size', CppInt64()),
        C('alloc_count', CppInt64()),
        C('cumulative_alloc_count', CppInt64()),
        C('alloc_size', CppInt64()),
        C('cumulative_alloc_size', CppInt64()),
        C('parent_id', CppOptional(CppSelfTableId())),
        C('source_file', CppOptional(CppString())),
        C('line_number', CppOptional(CppUint32())),
        C('upid_group', CppOptional(CppString())),
    ],
    tabledoc=TableDoc(
        doc='''
            Table used to render flamegraphs. This gives cumulative sizes of
            nodes in the flamegraph.

            WARNING: This is experimental and the API is subject to change.
        ''',
        group='Callstack profilers',
        columns={
            'ts': '''''',
            'upid': '''''',
            'profile_type': '''''',
            'focus_str': '''''',
            'depth': '''''',
            'name': '''''',
            'map_name': '''''',
            'count': '''''',
            'cumulative_count': '''''',
            'size': '''''',
            'cumulative_size': '''''',
            'alloc_count': '''''',
            'cumulative_alloc_count': '''''',
            'alloc_size': '''''',
            'cumulative_alloc_size': '''''',
            'parent_id': '''''',
            'source_file': '''''',
            'line_number': '''''',
            'upid_group': ''''''
        }))

HEAP_GRAPH_CLASS_TABLE = Table(
    class_name='HeapGraphClassTable',
    sql_name='heap_graph_class',
    columns=[
        C('name', CppString()),
        C('deobfuscated_name', CppOptional(CppString())),
        C('location', CppOptional(CppString())),
        C('superclass_id', CppOptional(CppSelfTableId())),
        C('classloader_id', CppOptional(CppUint32())),
        C('kind', CppString()),
    ],
    tabledoc=TableDoc(
        doc='''''',
        group='ART Heap Graphs',
        columns={
            'name':
                '''(potentially obfuscated) name of the class.''',
            'deobfuscated_name':
                '''if class name was obfuscated and deobfuscation map
for it provided, the deobfuscated name.''',
            'location':
                '''the APK / Dex / JAR file the class is contained in.

classloader_id should really be HeapGraphObject::id, but that would
create a loop, which is currently not possible.
TODO(lalitm): resolve this''',
            'superclass_id':
                '''''',
            'classloader_id':
                '''''',
            'kind':
                ''''''
        }))

HEAP_GRAPH_OBJECT_TABLE = Table(
    class_name='HeapGraphObjectTable',
    sql_name='heap_graph_object',
    columns=[
        C('upid', CppUint32()),
        C('graph_sample_ts', CppInt64()),
        C('self_size', CppInt64()),
        C('native_size', CppInt64()),
        C('reference_set_id', CppOptional(CppUint32()), flags=ColumnFlag.DENSE),
        C('reachable', CppInt32()),
        C('type_id', CppTableId(HEAP_GRAPH_CLASS_TABLE)),
        C('root_type', CppOptional(CppString())),
        C('root_distance', CppInt32(), flags=ColumnFlag.HIDDEN),
    ],
    tabledoc=TableDoc(
        doc='''
          The objects on the Dalvik heap.

All rows with the same (upid, graph_sample_ts) are one dump.
        ''',
        group='ART Heap Graphs',
        columns={
            'upid':
                '''UniquePid of the target.''',
            'graph_sample_ts':
                '''timestamp this dump was taken at.''',
            'self_size':
                '''size this object uses on the Java Heap.''',
            'native_size':
                '''approximate amount of native memory used by this object,
as reported by libcore.util.NativeAllocationRegistry.size.''',
            'reference_set_id':
                '''join key with heap_graph_reference containing all
objects referred in this object's fields.''',
            'reachable':
                '''bool whether this object is reachable from a GC root. If
false, this object is uncollected garbage.''',
            'type_id':
                '''class this object is an instance of.''',
            'root_type':
                '''if not NULL, this object is a GC root.''',
            'root_distance':
                ''''''
        }))

HEAP_GRAPH_REFERENCE_TABLE = Table(
    class_name='HeapGraphReferenceTable',
    sql_name='heap_graph_reference',
    columns=[
        C('reference_set_id',
          CppUint32(),
          flags=ColumnFlag.SORTED | ColumnFlag.SET_ID),
        C('owner_id', CppTableId(HEAP_GRAPH_OBJECT_TABLE)),
        C('owned_id', CppOptional(CppTableId(HEAP_GRAPH_OBJECT_TABLE))),
        C('field_name', CppString()),
        C('field_type_name', CppString()),
        C('deobfuscated_field_name', CppOptional(CppString())),
    ],
    tabledoc=TableDoc(
        doc='''
          Many-to-many mapping between heap_graph_object.

This associates the object with given reference_set_id with the objects
that are referred to by its fields.
        ''',
        group='ART Heap Graphs',
        columns={
            'reference_set_id':
                '''join key to heap_graph_object.''',
            'owner_id':
                '''id of object that has this reference_set_id.''',
            'owned_id':
                '''id of object that is referred to.''',
            'field_name':
                '''the field that refers to the object. E.g. Foo.name.''',
            'field_type_name':
                '''the static type of the field. E.g. java.lang.String.''',
            'deobfuscated_field_name':
                '''if field_name was obfuscated and a
deobfuscation mapping was provided for it, the deobfuscated name.'''
        }))

VULKAN_MEMORY_ALLOCATIONS_TABLE = Table(
    class_name='VulkanMemoryAllocationsTable',
    sql_name='vulkan_memory_allocations',
    columns=[
        C('arg_set_id', CppOptional(CppUint32())),
        C('source', CppString()),
        C('operation', CppString()),
        C('timestamp', CppInt64()),
        C('upid', CppOptional(CppUint32())),
        C('device', CppOptional(CppInt64())),
        C('device_memory', CppOptional(CppInt64())),
        C('memory_type', CppOptional(CppUint32())),
        C('heap', CppOptional(CppUint32())),
        C('function_name', CppOptional(CppString())),
        C('object_handle', CppOptional(CppInt64())),
        C('memory_address', CppOptional(CppInt64())),
        C('memory_size', CppOptional(CppInt64())),
        C('scope', CppString()),
    ],
    tabledoc=TableDoc(
        doc='''''',
        group='Misc',
        columns={
            'arg_set_id': '''''',
            'source': '''''',
            'operation': '''''',
            'timestamp': '''''',
            'upid': '''''',
            'device': '''''',
            'device_memory': '''''',
            'memory_type': '''''',
            'heap': '''''',
            'function_name': '''''',
            'object_handle': '''''',
            'memory_address': '''''',
            'memory_size': '''''',
            'scope': ''''''
        }))

GPU_COUNTER_GROUP_TABLE = Table(
    class_name='GpuCounterGroupTable',
    sql_name='gpu_counter_group',
    columns=[
        C('group_id', CppInt32()),
        C('track_id', CppTableId(TRACK_TABLE)),
    ],
    tabledoc=TableDoc(
        doc='''''',
        group='Misc',
        columns={
            'group_id': '''''',
            'track_id': ''''''
        }))

# Keep this list sorted.
ALL_TABLES = [
    CPU_PROFILE_STACK_SAMPLE_TABLE,
    EXPERIMENTAL_FLAMEGRAPH_NODES_TABLE,
    GPU_COUNTER_GROUP_TABLE,
    HEAP_GRAPH_CLASS_TABLE,
    HEAP_GRAPH_OBJECT_TABLE,
    HEAP_GRAPH_REFERENCE_TABLE,
    HEAP_PROFILE_ALLOCATION_TABLE,
    PACKAGE_LIST_TABLE,
    PERF_SAMPLE_TABLE,
    PROFILER_SMAPS_TABLE,
    STACK_PROFILE_CALLSITE_TABLE,
    STACK_PROFILE_FRAME_TABLE,
    STACK_PROFILE_MAPPING_TABLE,
    STACK_SAMPLE_TABLE,
    SYMBOL_TABLE,
    VULKAN_MEMORY_ALLOCATIONS_TABLE,
]
