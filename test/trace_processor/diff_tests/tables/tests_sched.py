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

from python.generators.diff_tests.testing import Path, DataPath, Metric
from python.generators.diff_tests.testing import Csv, Json, TextProto
from python.generators.diff_tests.testing import DiffTestBlueprint
from python.generators.diff_tests.testing import TestSuite


class TablesSched(TestSuite):
  # Sched table
  def test_synth_1_filter_sched(self):
    return DiffTestBlueprint(
        trace=Path('../common/synth_1.py'),
        query="""
        SELECT ts, cpu, dur FROM sched
        WHERE
          cpu = 1
          AND dur > 50
          AND dur <= 100
          AND ts >= 100
          AND ts <= 400;
        """,
        out=Csv("""
        "ts","cpu","dur"
        170,1,80
        """))

  def test_android_sched_and_ps_b119496959(self):
    return DiffTestBlueprint(
        trace=DataPath('android_sched_and_ps.pb'),
        query="""
        SELECT ts, cpu FROM sched WHERE ts >= 81473797418963 LIMIT 10;
        """,
        out=Csv("""
        "ts","cpu"
        81473797824982,3
        81473797942847,3
        81473798135399,0
        81473798786857,2
        81473798875451,3
        81473799019930,2
        81473799079982,0
        81473800089357,3
        81473800144461,3
        81473800441805,3
        """))

  def test_android_sched_and_ps_b119301023(self):
    return DiffTestBlueprint(
        trace=DataPath('android_sched_and_ps.pb'),
        query="""
        SELECT ts FROM sched
        WHERE ts > 0.1 + 1e9
        LIMIT 10;
        """,
        out=Csv("""
        "ts"
        81473010031230
        81473010109251
        81473010121751
        81473010179772
        81473010203886
        81473010234720
        81473010278522
        81473010308470
        81473010341386
        81473010352792
        """))

  def test_sched_wakeup(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT * FROM spurious_sched_wakeup
        ORDER BY ts LIMIT 10
        """,
        out=Csv("""
        "id","type","ts","thread_state_id","irq_context","utid","waker_utid"
        0,"spurious_sched_wakeup",1735850782904,422,0,230,1465
        1,"spurious_sched_wakeup",1736413914899,884,0,230,1467
        2,"spurious_sched_wakeup",1736977755745,1295,0,230,1469
        3,"spurious_sched_wakeup",1737046900004,1468,0,1472,1473
        4,"spurious_sched_wakeup",1737047159060,1497,0,1474,1472
        5,"spurious_sched_wakeup",1737081636170,2987,0,1214,1319
        6,"spurious_sched_wakeup",1737108696536,5005,0,501,557
        7,"spurious_sched_wakeup",1737153309978,6426,0,11,506
        8,"spurious_sched_wakeup",1737165240546,6910,0,565,499
        9,"spurious_sched_wakeup",1737211563344,8994,0,178,1195
        """))

  def test_raw_common_flags(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT * FROM raw WHERE common_flags != 0 ORDER BY ts LIMIT 10
        """,
        out=Csv("""
        "id","type","ts","name","cpu","utid","arg_set_id","common_flags"
        3,"ftrace_event",1735489788930,"sched_waking",0,300,4,1
        4,"ftrace_event",1735489812571,"sched_waking",0,300,5,1
        5,"ftrace_event",1735489833977,"sched_waking",1,305,6,1
        8,"ftrace_event",1735489876788,"sched_waking",1,297,9,1
        9,"ftrace_event",1735489879097,"sched_waking",0,304,10,1
        12,"ftrace_event",1735489933912,"sched_waking",0,428,13,1
        14,"ftrace_event",1735489972385,"sched_waking",1,232,15,1
        17,"ftrace_event",1735489999987,"sched_waking",1,232,15,1
        19,"ftrace_event",1735490039439,"sched_waking",1,298,18,1
        20,"ftrace_event",1735490042084,"sched_waking",1,298,19,1
        """))

  def test_thread_executing_span_graph(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT
          ts,
          dur,
          tid,
          pid,
          thread_name,
          process_name,
          waker_thread_name,
          waker_process_name,
          blocked_dur,
          blocked_state,
          blocked_function
        FROM experimental_thread_executing_span_graph
          WHERE blocked_function IS NOT NULL
        ORDER BY ts, tid
        LIMIT 10
        """,
        out=Csv("""
        "ts","dur","tid","pid","thread_name","process_name","waker_thread_name","waker_process_name","blocked_dur","blocked_state","blocked_function"
        1735842234188,283571,122,122,"kworker/1:2","kworker/1:2-events","adbd","/apex/com.android.adbd/bin/adbd",351402620,"I","worker_thread"
        1735843726296,8545303,122,122,"kworker/1:2","kworker/1:2-events","adbd","/apex/com.android.adbd/bin/adbd",1208537,"I","worker_thread"
        1735850643698,16245,240,240,"kworker/0:3","kworker/0:3-events","shell svc 3474","/apex/com.android.adbd/bin/adbd",154087,"I","worker_thread"
        1735851953029,554638012,240,240,"kworker/0:3","kworker/0:3-events","adbd","/apex/com.android.adbd/bin/adbd",1103252,"I","worker_thread"
        1735886367018,191863,122,122,"kworker/1:2","kworker/1:2-events","adbd","/apex/com.android.adbd/bin/adbd",34095419,"I","worker_thread"
        1736125372478,52493,122,122,"kworker/1:2","kworker/1:2-events","kworker/0:3","kworker/0:3-events",238813597,"I","worker_thread"
        1736405409972,278036,122,122,"kworker/1:2","kworker/1:2-events","adbd","/apex/com.android.adbd/bin/adbd",279985001,"I","worker_thread"
        1736406817672,7959441,122,122,"kworker/1:2","kworker/1:2-events","adbd","/apex/com.android.adbd/bin/adbd",1129664,"I","worker_thread"
        1736413734042,25870,240,240,"kworker/0:3","kworker/0:3-events","shell svc 3476","/apex/com.android.adbd/bin/adbd",7143001,"I","worker_thread"
        1736413763072,31692550,14,14,"rcu_preempt","rcu_preempt","shell svc 3476","/apex/com.android.adbd/bin/adbd",4413060,"I","rcu_gp_fqs_loop"
        """))

  def test_thread_executing_span_graph_contains_forked_states(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT
          ts,
          dur,
          tid,
          pid,
          thread_name,
          process_name,
          waker_thread_name,
          waker_process_name,
          blocked_dur,
          blocked_state,
          blocked_function
        FROM experimental_thread_executing_span_graph
          WHERE id = 375
        """,
        out=Csv("""
        "ts","dur","tid","pid","thread_name","process_name","waker_thread_name","waker_process_name","blocked_dur","blocked_state","blocked_function"
        1735842081507,293868,3475,527,"shell svc 3474","/apex/com.android.adbd/bin/adbd","adbd","/apex/com.android.adbd/bin/adbd","[NULL]","[NULL]","[NULL]"
        """))

  def test_thread_executing_span_internal_runnable_state_has_no_running(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT COUNT(*) AS count FROM internal_runnable_state WHERE state = 'Running'
        """,
        out=Csv("""
        "count"
        0
        """))

  def test_thread_executing_span_graph_has_no_null_dur(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT ts,dur FROM experimental_thread_executing_span_graph
          WHERE dur IS NULL OR ts IS NULL
        """,
        out=Csv("""
        "ts","dur"
        """))

  def test_thread_executing_span_graph_accepts_null_irq_context(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_switch_original.pb'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT COUNT(*) AS count FROM experimental_thread_executing_span_graph
        """,
        out=Csv("""
        "count"
        25
        """))

  def test_thread_executing_span_descendants_null(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT
          ts,
          dur,
          tid,
          pid,
          thread_name,
          process_name,
          waker_thread_name,
          waker_process_name,
          blocked_dur,
          blocked_state,
          blocked_function,
          depth,
          is_root
        FROM EXPERIMENTAL_THREAD_EXECUTING_SPAN_DESCENDANTS(NULL)
        ORDER BY depth DESC, ts, tid
        LIMIT 10
        """,
        out=Csv("""
        "ts","dur","tid","pid","thread_name","process_name","waker_thread_name","waker_process_name","blocked_dur","blocked_state","blocked_function","depth","is_root"
        1740321632480,20897,404,398,"binder:398_2","/apex/com.android.os.statsd/bin/statsd","statsd.writer","/apex/com.android.os.statsd/bin/statsd",64173354,"S","[NULL]",424,0
        1740470009095,113509,3494,3487,"HeapTaskDaemon","com.android.providers.media.module","AsyncTask #1","com.android.providers.media.module",1204928,"S","[NULL]",424,0
        1740470126280,60885652,3494,3487,"HeapTaskDaemon","com.android.providers.media.module","AsyncTask #1","com.android.providers.media.module",3676,"S","[NULL]",424,0
        1740321596028,46679,633,398,"statsd.writer","/apex/com.android.os.statsd/bin/statsd","mediametrics","media.metrics",64143546,"S","[NULL]",423,0
        1740468702535,1449612,3548,3487,"AsyncTask #1","com.android.providers.media.module","HeapTaskDaemon","com.android.providers.media.module",1003391,"S","[NULL]",423,0
        1740321315576,62532,2161,553,"binder:553_7","/system/bin/mediaserver","binder:551_4","media.extractor",63953635,"S","[NULL]",422,0
        1740321322095,60476,553,553,"mediaserver","/system/bin/mediaserver","binder:551_4","media.extractor","[NULL]","[NULL]","[NULL]",422,0
        1740321326214,144263,2135,553,"binder:553_4","/system/bin/mediaserver","binder:551_4","media.extractor","[NULL]","[NULL]","[NULL]",422,0
        1740321344727,346525,552,552,"mediametrics","media.metrics","binder:551_4","media.extractor",63860347,"S","[NULL]",422,0
        1740419776108,13020460,3494,3487,"HeapTaskDaemon","com.android.providers.media.module","AsyncTask #1","com.android.providers.media.module",597159,"S","[NULL]",422,0
        """))

  def test_thread_executing_span_ancestors_null(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT
          ts,
          dur,
          tid,
          pid,
          thread_name,
          process_name,
          waker_thread_name,
          waker_process_name,
          blocked_dur,
          blocked_state,
          blocked_function,
          height,
          is_leaf,
          leaf_ts,
          leaf_blocked_dur,
          leaf_blocked_state,
          leaf_blocked_function
        FROM EXPERIMENTAL_THREAD_EXECUTING_SPAN_ANCESTORS(NULL)
        WHERE leaf_blocked_function IS NOT NULL
        ORDER BY height DESC, ts, tid
        LIMIT 10
        """,
        out=Csv("""
        "ts","dur","tid","pid","thread_name","process_name","waker_thread_name","waker_process_name","blocked_dur","blocked_state","blocked_function","height","is_leaf","leaf_ts","leaf_blocked_dur","leaf_blocked_state","leaf_blocked_function"
        1737212166776,2751675192,506,506,"kworker/u5:3","kworker/u5:3-erofs_unzipd","binder:243_4","/system/bin/vold","[NULL]","[NULL]","[NULL]",265,0,1740313970400,386080273,"I","worker_thread"
        1739963731743,267784,642,642,"system_server","system_server","kworker/u5:3","kworker/u5:3-erofs_unzipd",4725094,"D","filemap_fault",264,0,1740313970400,386080273,"I","worker_thread"
        1739963925635,1771245,1934,642,"binder:642_E","system_server","system_server","system_server",4766105,"S","[NULL]",263,0,1740313970400,386080273,"I","worker_thread"
        1739965371379,245311,642,642,"system_server","system_server","binder:642_E","system_server",1371852,"S","[NULL]",262,0,1740313970400,386080273,"I","worker_thread"
        1739965558519,326825,3500,3487,"binder:3487_3","com.android.providers.media.module","system_server","system_server",9183650,"S","[NULL]",261,0,1740313970400,386080273,"I","worker_thread"
        1739965848075,548636,3487,3487,"rs.media.module","com.android.providers.media.module","binder:3487_3","com.android.providers.media.module",6774461,"S","[NULL]",260,0,1740313970400,386080273,"I","worker_thread"
        1739966186324,1192880,3548,3487,"AsyncTask #1","com.android.providers.media.module","rs.media.module","com.android.providers.media.module","[NULL]","[NULL]","[NULL]",259,0,1740313970400,386080273,"I","worker_thread"
        1739967354198,311116,2721,642,"binder:642_13","system_server","AsyncTask #1","com.android.providers.media.module",2845516,"S","[NULL]",258,0,1740313970400,386080273,"I","worker_thread"
        1739967648689,61753222,3548,3487,"AsyncTask #1","com.android.providers.media.module","binder:642_13","system_server",269485,"S","[NULL]",257,0,1740313970400,386080273,"I","worker_thread"
        1740029390694,1179377,2721,642,"binder:642_13","system_server","AsyncTask #1","com.android.providers.media.module",4500139,"S","[NULL]",256,0,1740313970400,386080273,"I","worker_thread"
        """))

  def test_thread_executing_span_descendants_id(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT
          thread_name,
          waker_thread_name,
          depth,
          is_root,
          COUNT(thread_name) AS count
        FROM EXPERIMENTAL_THREAD_EXECUTING_SPAN_DESCENDANTS(22886)
        GROUP BY 1,2,3,4
        ORDER BY depth
        """,
        out=Csv("""
        "thread_name","waker_thread_name","depth","is_root","count"
        "SensorService","android.hardwar",0,0,1
        "android.ui","SensorService",1,0,1
        "system_server","SensorService",1,0,1
        """))

  def test_thread_executing_span_ancestors_id(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT
          thread_name,
          waker_thread_name,
          height,
          is_leaf
        FROM EXPERIMENTAL_THREAD_EXECUTING_SPAN_ANCESTORS(15601) ORDER BY height
        """,
        out=Csv("""
        "thread_name","waker_thread_name","height","is_leaf"
        "rcu_exp_gp_kthr","binder:243_4",0,1
        "binder:243_4","init",1,0
        """))

  def test_thread_executing_span_from_non_sleep_thread_state(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT EXPERIMENTAL_THREAD_EXECUTING_SPAN_ID_FROM_THREAD_STATE_ID(16170) AS thread_executing_span_id
        """,
        out=Csv("""
        "thread_executing_span_id"
        16022
        """))

  def test_thread_executing_span_from_sleep_thread_state(self):
    return DiffTestBlueprint(
        trace=DataPath('sched_wakeup_trace.atr'),
        query="""
        SELECT IMPORT('experimental.thread_executing_span');
        SELECT EXPERIMENTAL_THREAD_EXECUTING_SPAN_ID_FROM_THREAD_STATE_ID(15957) AS thread_executing_span_id
        """,
        out=Csv("""
        "thread_executing_span_id"
        "[NULL]"
        """))
