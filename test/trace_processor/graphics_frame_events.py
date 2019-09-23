#!/usr/bin/python
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

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.abspath(__file__))))
import synth_common

trace = synth_common.create_trace()
trace.add_buffer_event_packet(ts=1, buffer_id=1, layer_name="layer1", frame_number=11, event_type=1, duration=6)
trace.add_buffer_event_packet(ts=2, buffer_id=2, layer_name="layer2", frame_number=11, event_type=2, duration=7)
trace.add_buffer_event_packet(ts=3, buffer_id=3, layer_name="layer3", frame_number=11, event_type=3, duration=8)
trace.add_buffer_event_packet(ts=4, buffer_id=4, layer_name="layer4", frame_number=11, event_type=4, duration=9)
trace.add_buffer_event_packet(ts=5, buffer_id=5, layer_name="layer5", frame_number=11, event_type=5, duration=10)
# Repeat some layers
trace.add_buffer_event_packet(ts=6, buffer_id=1, layer_name="layer1", frame_number=12, event_type=1, duration=11)
trace.add_buffer_event_packet(ts=7, buffer_id=2, layer_name="layer2", frame_number=12, event_type=2, duration=12)
trace.add_buffer_event_packet(ts=8, buffer_id=3, layer_name="layer3", frame_number=12, event_type=3, duration=13)
# Missing id.
trace.add_buffer_event_packet(ts=6, buffer_id=-1, layer_name="layer6", frame_number=13, event_type=6, duration=11)
# Missing type.
trace.add_buffer_event_packet(ts=7, buffer_id=7, layer_name="layer7", frame_number=13, event_type=-1, duration=12)

print(trace.trace.SerializeToString())
