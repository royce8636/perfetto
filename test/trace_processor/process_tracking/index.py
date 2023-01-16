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


class DiffTestModule_Process_tracking(DiffTestModule):

  def test_process_tracking(self):
    return DiffTestBlueprint(
        trace=Path('synth_process_tracking.py'),
        query=Path('../common/process_tracking_test.sql'),
        out=Csv("""
"tid","pid","pname","tname"
10,10,"process1","p1-t0"
11,"[NULL]","[NULL]","p1-t1"
12,10,"process1","p1-t2"
20,20,"process_2","p2-t0"
21,20,"process_2","p2-t1"
22,20,"process_2","p2-t2"
30,30,"process_3","p3-t0"
31,30,"process_3","p3-t1"
31,40,"process_4","p4-t1"
32,30,"process_3","p3-t2"
33,30,"process_3","p3-t3"
34,30,"process_3","p3-t4"
40,40,"process_4","p4-t0"
"""))

  def test_process_tracking_process_tracking_short_lived_1(self):
    return DiffTestBlueprint(
        trace=Path('process_tracking_short_lived_1.py'),
        query=Path('../common/process_tracking_test.sql'),
        out=Csv("""
"tid","pid","pname","tname"
10,10,"parent","parent"
11,11,"child","child"
"""))

  def test_process_tracking_process_tracking_short_lived_2(self):
    return DiffTestBlueprint(
        trace=Path('process_tracking_short_lived_2.py'),
        query=Path('../common/process_tracking_test.sql'),
        out=Csv("""
"tid","pid","pname","tname"
10,10,"parent","parent"
11,11,"true_name","true_name"
"""))

  def test_process_tracking_uid(self):
    return DiffTestBlueprint(
        trace=Path('synth_process_tracking.py'),
        query=Path('process_tracking_uid_test.sql'),
        out=Csv("""
"pid","uid"
0,"[NULL]"
10,1001
20,1002
30,"[NULL]"
40,"[NULL]"
"""))

  def test_process_tracking_process_tracking_exec(self):
    return DiffTestBlueprint(
        trace=Path('process_tracking_exec.py'),
        query=Path('../common/process_tracking_test.sql'),
        out=Csv("""
"tid","pid","pname","tname"
10,10,"parent","parent"
11,11,"true_process_name","true_name"
"""))

  def test_process_parent_pid_process_parent_pid_tracking_1(self):
    return DiffTestBlueprint(
        trace=Path('process_parent_pid_tracking_1.py'),
        query=Path('process_parent_pid_test.sql'),
        out=Csv("""
"child_pid","parent_pid"
10,0
20,10
"""))

  def test_process_parent_pid_process_parent_pid_tracking_2(self):
    return DiffTestBlueprint(
        trace=Path('process_parent_pid_tracking_2.py'),
        query=Path('process_parent_pid_test.sql'),
        out=Csv("""
"child_pid","parent_pid"
10,0
20,10
"""))

  def test_process_tracking_reused_thread_print(self):
    return DiffTestBlueprint(
        trace=Path('reused_thread_print.py'),
        query=Path('../common/process_tracking_test.sql'),
        out=Csv("""
"tid","pid","pname","tname"
10,10,"parent","[NULL]"
11,11,"short_lived","[NULL]"
11,10,"parent","true_name"
"""))

  def test_slice_with_pid_sde_tracing_mark_write(self):
    return DiffTestBlueprint(
        trace=Path('sde_tracing_mark_write.textproto'),
        query=Path('slice_with_pid_test.sql'),
        out=Csv("""
"name","dur","tid","pid"
"test_event",1,403,403
"""))

  def test_unknown_thread_name_tracking(self):
    return DiffTestBlueprint(
        trace=Path('unknown_thread_name.systrace'),
        query=Path('../common/process_tracking_test.sql'),
        out=Csv("""
"tid","pid","pname","tname"
19999,"[NULL]","[NULL]","real_name"
"""))
