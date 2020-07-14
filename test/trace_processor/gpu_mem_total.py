#!/usr/bin/python
# Copyright (C) 2020 The Android Open Source Project
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
sys.path.append(path.dirname(path.dirname(path.abspath(__file__))))
import synth_common

trace = synth_common.create_trace()
trace.add_packet()
trace.add_process(0, 0, "global")
trace.add_process(1, 0, "app_1")

# Global gpu_mem_total event
trace.add_ftrace_packet(cpu=0)
trace.add_gpu_mem_total(pid=0, ts=0, size=123)
trace.add_gpu_mem_total(pid=0, ts=5, size=256)
trace.add_gpu_mem_total(pid=0, ts=10, size=123)

# pid = 1
trace.add_ftrace_packet(cpu=1)
trace.add_gpu_mem_total(pid=1, ts=0, size=100)
trace.add_gpu_mem_total(pid=1, ts=5, size=233)
trace.add_gpu_mem_total(pid=1, ts=10, size=0)

print(trace.trace.SerializeToString())
