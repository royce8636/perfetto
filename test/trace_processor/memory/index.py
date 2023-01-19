#!/usr/bin/env python3
# Copyright (C) 2023 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License a
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from python.generators.diff_tests.testing import Path, Metric
from python.generators.diff_tests.testing import Csv, Json, TextProto
from python.generators.diff_tests.testing import DiffTestBlueprint
from python.generators.diff_tests.testing import DiffTestModule


class DiffTestModule_Memory(DiffTestModule):

  def test_android_mem_counters(self):
    return DiffTestBlueprint(
        trace=Path('../../data/memory_counters.pb'),
        query=Metric('android_mem'),
        out=Path('android_mem_counters.out'))

  def test_trace_metadata(self):
    return DiffTestBlueprint(
        trace=Path('../../data/memory_counters.pb'),
        query=Metric('trace_metadata'),
        out=Path('trace_metadata.out'))

  def test_android_mem_by_priority(self):
    return DiffTestBlueprint(
        trace=Path('android_mem_by_priority.py'),
        query=Metric('android_mem'),
        out=Path('android_mem_by_priority.out'))

  def test_android_mem_lmk(self):
    return DiffTestBlueprint(
        trace=Path('android_systrace_lmk.py'),
        query=Metric('android_lmk'),
        out=TextProto(r"""
android_lmk {
  total_count: 1
    by_oom_score {
    oom_score_adj: 900
    count: 1
  }
  oom_victim_count: 0
}
"""))

  def test_android_lmk_oom(self):
    return DiffTestBlueprint(
        trace=Path('../common/oom_kill.textproto'),
        query=Metric('android_lmk'),
        out=TextProto(r"""
android_lmk {
  total_count: 0
  oom_victim_count: 1
}"""))

  def test_android_mem_delta(self):
    return DiffTestBlueprint(
        trace=Path('android_mem_delta.py'),
        query=Metric('android_mem'),
        out=TextProto(r"""
android_mem {
  process_metrics {
    process_name: "com.my.pkg"
    total_counters {
      file_rss {
        min: 2000.0
        max: 10000.0
        avg: 6666.666666666667
        delta: 7000.0
      }
    }
  }
}
"""))

  def test_android_ion(self):
    return DiffTestBlueprint(
        trace=Path('android_ion.py'),
        query=Metric('android_ion'),
        out=TextProto(r"""
android_ion {
  buffer {
    name: "adsp"
    avg_size_bytes: 1000.0
    min_size_bytes: 1000.0
    max_size_bytes: 1100.0
    total_alloc_size_bytes: 1100.0
  }
  buffer {
    name: "system"
    avg_size_bytes: 1497.4874371859296
    min_size_bytes: 1000.0
    max_size_bytes: 2000.0
    total_alloc_size_bytes: 2000.0
  }
}
"""))

  def test_android_ion_stat(self):
    return DiffTestBlueprint(
        trace=Path('android_ion_stat.textproto'),
        query=Metric('android_ion'),
        out=TextProto(r"""
android_ion {
  buffer {
    name: "all"
    avg_size_bytes: 2000.0
    min_size_bytes: 1000.0
    max_size_bytes: 2000.0
    total_alloc_size_bytes: 1000.0
  }
}"""))

  def test_android_dma_heap_stat(self):
    return DiffTestBlueprint(
        trace=Path('android_dma_heap_stat.textproto'),
        query=Metric('android_dma_heap'),
        out=TextProto(r"""
android_dma_heap {
    avg_size_bytes: 2048.0
    min_size_bytes: 1024.0
    max_size_bytes: 2048.0
    total_alloc_size_bytes: 1024.0
}
"""))

  def test_android_dma_buffer_tracks(self):
    return DiffTestBlueprint(
        trace=Path('android_dma_heap_stat.textproto'),
        query="""
SELECT track.name, slice.ts, slice.dur, slice.name
FROM slice JOIN track ON slice.track_id = track.id
WHERE track.name = 'mem.dma_buffer';
""",
        out=Csv("""
"name","ts","dur","name"
"mem.dma_buffer",100,100,"1 kB"
"""))

  def test_android_fastrpc_dma_stat(self):
    return DiffTestBlueprint(
        trace=Path('android_fastrpc_dma_stat.textproto'),
        query=Metric('android_fastrpc'),
        out=TextProto(r"""
android_fastrpc {
  subsystem {
    name: "MDSP"
    avg_size_bytes: 2000.0
    min_size_bytes: 1000.0
    max_size_bytes: 2000.0
    total_alloc_size_bytes: 1000.0
  }
}
"""))

  def test_shrink_slab(self):
    return DiffTestBlueprint(
        trace=Path('shrink_slab.textproto'),
        query="""
SELECT ts, dur, name FROM slice WHERE name = 'mm_vmscan_shrink_slab';
""",
        out=Csv("""
"ts","dur","name"
36448185787847,692,"mm_vmscan_shrink_slab"
"""))

  def test_cma(self):
    return DiffTestBlueprint(
        trace=Path('cma.textproto'),
        query="""
SELECT ts, dur, name FROM slice WHERE name = 'mm_cma_alloc';
""",
        out=Csv("""
"ts","dur","name"
74288080958099,110151652,"mm_cma_alloc"
"""))
