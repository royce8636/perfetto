#!/usr/bin/env python
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

import argparse
import difflib
import glob
import os
import subprocess
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST_DATA_DIR = os.path.join(ROOT_DIR, "test", "trace_processor")

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--index', type=str, help='location of index file',
                      default=os.path.join(TEST_DATA_DIR, "index"))
  parser.add_argument('trace_processor', type=str,
                      help='location of trace processor binary')
  args = parser.parse_args()

  with open(args.index, 'r') as file:
    index_lines = file.readlines()

  test_failure = 0
  index_dir = os.path.dirname(args.index)
  for line in index_lines:
    [trace_fname, query_fname, expected_fname] = line.strip().split(' ')

    trace_path = os.path.abspath(os.path.join(index_dir, trace_fname))
    query_path = os.path.abspath(os.path.join(index_dir, query_fname))
    expected_path = os.path.abspath(os.path.join(index_dir, expected_fname))
    if not os.path.exists(trace_path):
      print("Trace file not found {}".format(trace_path))
      return 1
    elif not os.path.exists(query_path):
      print("Query file not found {}".format(query_path))
      return 1
    elif not os.path.exists(expected_path):
      print("Expected file not found {}".format(expected_path))
      return 1

    actual_raw = subprocess.check_output([
      args.trace_processor,
      '-q',
      query_path,
      trace_path
    ])
    actual = actual_raw.decode("utf-8")
    actual_lines = actual_raw.splitlines(True)

    with open(expected_path, "r") as expected_file:
      expected = expected_file.read()
      if expected != actual:
        sys.stderr.write(
          "Expected did not match actual for trace {} and query {}"
          .format(trace_path, query_path))

        expected_lines = expected.splitlines(True)
        diff = difflib.unified_diff(expected_lines, actual_lines,
                                    fromfile="expected", tofile="actual")
        for line in diff:
          sys.stderr.write(line)
        test_failure += 1

  if test_failure == 0:
    print("All tests passed successfully")
    return 0
  else:
    print("Total failures: {}".format(test_failure))
    return 1

if __name__ == '__main__':
  sys.exit(main())
