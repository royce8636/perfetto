#!/usr/bin/env python3
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

# Discarded events that do not get to GPU are invisible for UMA metric and
# therefore should be excluded in trace-based metric. This tests ensures that's
# the case.

from os import sys

import synth_common

from synth_common import ms_to_ns
trace = synth_common.create_trace()

from chrome_scroll_helper import ChromeScrollHelper

helper = ChromeScrollHelper(trace, start_id=1234, start_gesture_id=5678)

# First scroll
helper.begin(from_ms=0, dur_ms=10)
helper.update(from_ms=15, dur_ms=10)
helper.update(from_ms=30, dur_ms=10)
helper.end(from_ms=45, dur_ms=10)

# Second scroll
helper.begin(from_ms=60, dur_ms=10)
helper.update(from_ms=75, dur_ms=10)
helper.end(from_ms=90, dur_ms=10)

# Third scroll, will overlap with second scroll
helper.begin(from_ms=80, dur_ms=10)
helper.update(from_ms=95, dur_ms=10)
helper.end(from_ms=100, dur_ms=10)

# Fourth scroll, won't have a GestureScrollEnd value.
helper.begin(from_ms=120, dur_ms=10)
helper.update(from_ms=135, dur_ms=10)
helper.update(from_ms=150, dur_ms=10)
helper.update(from_ms=150, dur_ms=10)
helper.update(from_ms=180, dur_ms=10)

sys.stdout.buffer.write(trace.trace.SerializeToString())
