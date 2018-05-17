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

""" Script to check whether a given buildtool folder exists.

Prints a user-friendly message if it doesn't.
"""

import os
import sys

def main():
  if not os.path.exists(sys.argv[1]):
    err = '\x1b[31mCannot find %s/%s\nRun tools/install-build-deps --ui\x1b[0m'
    print >>sys.stderr,  err % (os.path.abspath('.'), sys.argv[1])
    return 127

if __name__ == '__main__':
  sys.exit(main())
