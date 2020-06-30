#!/usr/bin/env python3
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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import os
import sys

from proto_utils import create_message_factory, serialize_python_trace, serialize_textproto_trace


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--out',
      type=str,
      required=True,
      help='out directory to search for trace descriptor')
  parser.add_argument('trace_path', type=str, help='path of trace to serialize')
  args = parser.parse_args()

  trace_protos_path = os.path.join(args.out, 'gen', 'protos', 'perfetto',
                                   'trace')
  trace_descriptor_path = os.path.join(trace_protos_path, 'trace.descriptor')

  trace_path = args.trace_path

  if trace_path.endswith('.py'):
    serialize_python_trace(trace_descriptor_path, trace_path, sys.stdout.buffer)
  elif trace_path.endswith('.textproto'):
    serialize_textproto_trace(trace_descriptor_path, trace_path,
                              sys.stdout.buffer)
  else:
    raise RuntimeError('Invalid extension for unserialized trace file')

  return 0


if __name__ == '__main__':
  sys.exit(main())
