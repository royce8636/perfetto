#!/usr/bin/env python3
# Copyright (C) 2022 The Android Open Source Project
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
import os
import sys
import json
import re
from collections import defaultdict
from sql_modules_utils import *

# Creates a JSON file with documentation for stdlib files.

REPLACEMENT_HEADER = '''/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *******************************************************************************
 * AUTOGENERATED BY tools/gen_stdlib_docs - DO NOT EDIT
 *******************************************************************************
 */

 #include <string.h>
'''


def parse_columns(col_lines):
  cols = {}
  last_col_name = ""
  for line in col_lines:
    m = re.match(r'^-- @column[ \t]+(\w+)[ \t]+(.*)', line)
    if m:
      cols[m.group(1)] = m.group(2)
      last_col_name = m.group(1)
    else:
      line = line.strip('--').lstrip()
      if line:
        cols[last_col_name] += " " + line
  return cols


def parse_comment(comment_lines):
  text_lines = []
  for i, line in enumerate(comment_lines):
    # Break if columns definition started.
    m = re.match(r'^-- @column[ \t]+(\w+)[ \t]+(.*)', line)
    if m is not None:
      return dict(
          cols=parse_columns(comment_lines[i:]),
          desc=" ".join(text_lines).strip('\n').strip())

    line = line.strip('--').lstrip()
    if line:
      text_lines.append(line)
    if not line:
      text_lines.append('\n')
  raise ValueError('No columns found')


def parse_function_comment(comment):
  text = []
  next_block = 0

  # Fetch function description
  for i, line in enumerate(comment):
    if line.startswith('-- @arg'):
      next_block = i
      break

    line = line.strip('--').lstrip()
    if line:
      text.append(line)
    if not line:
      text.append('\n')

  # Fetch function args
  args = {}
  last_arg_name = ""
  for i, line in enumerate(comment[next_block:]):
    if line.startswith('-- @ret'):
      next_block = i
      break

    m = re.match(args_pattern(), line)
    if m:
      args[m.group(1)] = dict(type=m.group(2), desc=m.group(3))
      last_arg_name = m.group(1)
    else:
      line = line.strip('--').lstrip()
      if line:
        args[last_arg_name]['desc'] += " " + line

  # Fetch function ret
  ret_type, ret_desc = "", ""
  for line in comment[next_block:]:
    m = re.match(function_return_pattern(), line)
    if m:
      ret_type, ret_desc = m.group(1), m.group(2)
    else:
      line = line.strip('--').lstrip()
      if line:
        ret_desc += " " + line

  return dict(
      args=args,
      desc=" ".join(text).strip('\n').strip(),
      return_type=ret_type,
      return_desc=ret_desc)


def docs_for_obj_in_file(sql):
  line_to_match_dict = match_pattern(create_table_view_pattern(), sql)
  if not line_to_match_dict:
    return []

  lines = sql.split('\n')
  ret = []
  for line_id, match_groups in line_to_match_dict.items():
    name, obj_type = match_groups[1], match_groups[0]
    if re.match(r'^internal_.*', name):
      continue

    comment = fetch_comment(lines[line_id - 1::-1])
    ret.append(dict(name=name, type=obj_type, **parse_comment(comment)))

  return ret


def docs_for_functions_in_file(sql):
  line_to_match_dict = match_pattern(create_function_pattern(), sql)
  if not bool(line_to_match_dict):
    return []

  lines = sql.split('\n')
  ret = []
  for line_id, match_groups in line_to_match_dict.items():
    name = match_groups[0]
    if re.match(r'^INTERNAL_.*', name):
      continue

    comment = fetch_comment(lines[line_id - 1::-1])
    ret.append(dict(name=name, **parse_function_comment(comment)))
  return ret


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--json-out', required=True)
  parser.add_argument('--input-list-file')
  parser.add_argument('--root-dir', required=True)
  parser.add_argument('sql_files', nargs='*')
  args = parser.parse_args()

  if args.input_list_file and args.sql_files:
    print("Only one of --input-list-file and list of SQL files expected")
    return 1

  sql_files = []
  if args.input_list_file:
    with open(args.input_list_file, 'r') as input_list_file:
      for line in input_list_file.read().splitlines():
        sql_files.append(line)
  else:
    sql_files = args.sql_files

  # Extract the SQL output from each file.
  sql_outputs = {}
  for file_name in sql_files:
    with open(file_name, 'r') as f:
      relpath = os.path.relpath(file_name, args.root_dir)
      sql_outputs[relpath] = f.read()

  modules = defaultdict(list)

  # Add documentation from each file
  for path, sql in sql_outputs.items():
    module_name = path.split("/")[0]

    import_key = path.split(".sql")[0].replace("/", ".")
    if module_name == 'common':
      import_key = import_key.split(".", 1)[-1]

    file_dict = dict(
        import_key=import_key,
        imports=docs_for_obj_in_file(sql),
        functions=docs_for_functions_in_file(sql))
    modules[module_name].append(file_dict)

  with open(args.json_out, 'w+') as f:
    json.dump(modules, f, indent=4)

  return 0


if __name__ == '__main__':
  sys.exit(main())
