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


class DiffTestModule_Fuchsia(DiffTestModule):

  def test_fuchsia_smoke(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_trace.fxt'),
        query=Path('../common/smoke_test.sql'),
        out=Csv("""
"ts","cpu","dur","end_state","priority","tid"
19675868967,2,79022,"S",20,4344
19676000188,3,504797,"S",20,6547
19676504985,3,42877,"S",20,6525
19676582005,0,48467,"S",20,11566
19676989045,2,138116,"S",20,9949
19677162311,3,48655,"S",20,6525
19677305405,3,48814,"S",20,6525
19677412330,0,177220,"S",20,4344
19677680485,2,91422,"S",20,6537
19677791779,3,96082,"S",20,1680
"""))

  def test_fuchsia_smoke_slices(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_trace.fxt'),
        query=Path('../common/smoke_slices_test.sql'),
        out=Csv("""
"type","depth","count"
"thread_track",0,2153
"thread_track",1,1004
"""))

  def test_fuchsia_smoke_instants(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_trace.fxt'),
        query=Path('smoke_instants_test.sql'),
        out=Csv("""
"ts","name"
21442756010,"task_start"
21446583438,"task_end"
21448366538,"task_start"
21450363277,"task_end"
21454255741,"task_start"
21457834528,"task_end"
21459006408,"task_start"
21460601866,"task_end"
21461282720,"task_start"
21462998487,"task_end"
"""))

  def test_fuchsia_smoke_counters(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_trace.fxt'),
        query=Path('smoke_counters_test.sql'),
        out=Csv("""
"ts","value","name"
20329439768,30.331177,"cpu_usage:average_cpu_percentage"
21331281870,7.829745,"cpu_usage:average_cpu_percentage"
22332302017,9.669818,"cpu_usage:average_cpu_percentage"
23332974162,6.421237,"cpu_usage:average_cpu_percentage"
24333405767,12.079849,"cpu_usage:average_cpu_percentage"
"""))

  def test_fuchsia_smoke_flow(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_trace.fxt'),
        query=Path('smoke_flow_test.sql'),
        out=Csv("""
"id","slice_out","slice_in"
0,0,1
1,2,3
2,4,5
3,6,7
4,8,9
5,10,11
6,12,13
7,14,15
8,16,17
9,18,19
"""))

  def test_fuchsia_smoke_type(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_trace.fxt'),
        query=Path('smoke_type_test.sql'),
        out=Csv("""
"id","name","type"
0,"[NULL]","thread_track"
1,"[NULL]","thread_track"
2,"[NULL]","thread_track"
3,"[NULL]","thread_track"
4,"[NULL]","thread_track"
5,"cpu_usage:average_cpu_percentage","process_counter_track"
6,"[NULL]","thread_track"
7,"[NULL]","thread_track"
8,"[NULL]","thread_track"
9,"[NULL]","thread_track"
"""))

  def test_fuchsia_workstation_smoke_slices(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_workstation.fxt'),
        query=Path('../common/smoke_slices_test.sql'),
        out=Path('fuchsia_workstation_smoke_slices.out'))

  def test_fuchsia_workstation_smoke_args(self):
    return DiffTestBlueprint(
        trace=Path('../../data/fuchsia_workstation.fxt'),
        query=Path('smoke_args_test.sql'),
        out=Csv("""
"key","COUNT(*)"
"Dart Arguments",3
"Escher frame number",33
"Expected presentation time",17
"Frame number",33
"MinikinFontsCount",2
"Predicted frame duration(ms)",21
"Render time(ms)",21
"Timestamp",917
"Update time(ms)",21
"Vsync interval",900
"""))
