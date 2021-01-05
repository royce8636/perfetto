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
""" Compiles the stress_test configs protos and bundles in a .h C++ array.

This scripts takes all the configs in /test/stress_test/configs, compiles them
with protoc and generates a C++ header which contains the configs' names and
proto-encoded bytes.

This is invoked by the build system and is used by the stress_test runner. The
goal is making the stress_test binary hermetic and not depend on the repo.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
import os
import sys
import argparse
import shutil
import subprocess

CUR_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.dirname(os.path.dirname(CUR_DIR))
CONFIGS_DIR = os.path.join(CUR_DIR, 'configs')


def find_protoc():
  for root, _, files in os.walk(os.path.join(ROOT_DIR, 'out')):
    if 'protoc' in files:
      return os.path.join(root, 'protoc')
  return None


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--protoc')
  parser.add_argument('--out', required=True)
  parser.add_argument('cfgfiles', nargs='+')
  args = parser.parse_args()

  protoc = args.protoc or find_protoc()
  assert protoc, 'protoc not found, pass --protoc /path/to/protoc'
  assert os.path.exists(protoc), '{} does not exist'.format(protoc)
  if protoc is not args.protoc:
    print('Using protoc: {}'.format(protoc))

  blobs = {}
  for cfg_path in args.cfgfiles:
    cfg_path = cfg_path.replace('\\', '/')
    cfg_name = os.path.splitext(cfg_path)[0].split('/')[-1]
    with open(cfg_path, 'r') as in_file:
      compiled_proto = subprocess.check_output([
          protoc,
          '--encode=perfetto.protos.StressTestConfig',
          '--proto_path=' + ROOT_DIR,
          os.path.join(ROOT_DIR, 'protos', 'perfetto', 'config',
                       'stress_test_config.proto'),
      ],
                                               stdin=in_file)
    blobs[cfg_name] = bytearray(compiled_proto)

  # Write the C++ header file
  fout = open(args.out, 'wb')
  include_guard = args.out.replace('/', '_').replace('.', '_').upper() + '_'
  fout.write("""
#ifndef {include_guard}
#define {include_guard}

#include <stddef.h>
#include <stdint.h>

// This file was autogenerated by ${gen_script}. Do not edit.

namespace perfetto {{
namespace {{

struct StressTestConfigBlob {{
  const char* name;
  const uint8_t* data;
  size_t size;
}};\n\n""".format(
      gen_script=__file__,
      include_guard=include_guard,
  ).encode())

  configs_arr = '\nconst StressTestConfigBlob kStressTestConfigs[] = {\n'
  for cfg_name, blob in blobs.items():
    arr_str = ','.join(str(b) for b in blob)
    line = 'const uint8_t _config_%s[]{%s};\n' % (cfg_name, arr_str)
    fout.write(line.encode())
    configs_arr += '  {{"{n}", _config_{n}, sizeof(_config_{n})}},\n'.format(
        n=cfg_name)
  configs_arr += '};\n'
  fout.write(configs_arr.encode())
  fout.write("""
}  // namespace
}  // namespace perfetto
#endif\n""".encode())
  fout.close()


if __name__ == '__main__':
  exit(main())
