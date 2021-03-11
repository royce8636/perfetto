#!/usr/bin/env python3
# Copyright (C) 2018 The Android Open Source Project
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

from os import sys, path

import synth_common

APP_PID = 3
APP_TID = 1
SYSTEM_SERVER_PID = 2
SYSTEM_SERVER_TID = 2
LAUNCH_START_TS = 100
LAUNCH_END_TS = 200

trace = synth_common.create_trace()
trace.add_packet()
trace.add_process(1, 0, 'init')
trace.add_process(SYSTEM_SERVER_PID, 1, 'system_server')
trace.add_process(APP_PID, 1, 'com.some.app')

trace.add_ftrace_packet(cpu=0)
# Start intent.
trace.add_atrace_begin(
    ts=LAUNCH_START_TS,
    pid=SYSTEM_SERVER_PID,
    tid=SYSTEM_SERVER_TID,
    buf='MetricsLogger:launchObserverNotifyIntentStarted')
trace.add_atrace_end(
    ts=LAUNCH_START_TS + 1, tid=SYSTEM_SERVER_TID, pid=SYSTEM_SERVER_PID)

# System server launching the app.
trace.add_atrace_async_begin(
    ts=LAUNCH_START_TS + 2,
    pid=SYSTEM_SERVER_PID,
    tid=SYSTEM_SERVER_TID,
    buf='launching: com.some.app')

# OpenDex slices within the startup.
trace.add_atrace_begin(
    ts=150, pid=APP_PID, tid=APP_TID, buf='OpenDexFilesFromOat(something)')
trace.add_atrace_end(ts=165, pid=APP_PID, tid=APP_TID)

trace.add_atrace_begin(
    ts=170, pid=APP_PID, tid=APP_TID, buf='OpenDexFilesFromOat(something else)')
trace.add_atrace_end(ts=175, pid=APP_PID, tid=APP_TID)

# OpenDex slice outside the startup.
trace.add_atrace_begin(
    ts=5, pid=APP_PID, tid=APP_TID, buf='OpenDexFilesFromOat(nothing)')
trace.add_atrace_end(ts=35, pid=APP_PID, tid=APP_TID)

trace.add_atrace_async_end(
    ts=LAUNCH_END_TS,
    tid=SYSTEM_SERVER_TID,
    pid=SYSTEM_SERVER_PID,
    buf='launching: com.some.app')

# Intent successful.
trace.add_atrace_begin(
    ts=LAUNCH_END_TS + 1,
    pid=SYSTEM_SERVER_PID,
    tid=SYSTEM_SERVER_TID,
    buf='MetricsLogger:launchObserverNotifyActivityLaunchFinished')
trace.add_atrace_end(
    ts=LAUNCH_END_TS + 2, tid=SYSTEM_SERVER_TID, pid=SYSTEM_SERVER_PID)

sys.stdout.buffer.write(trace.trace.SerializeToString())
