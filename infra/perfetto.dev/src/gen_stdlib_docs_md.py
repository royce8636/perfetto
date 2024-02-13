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
# disibuted under the License is disibuted on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import sys
import json
from typing import Any, List, Dict


# Escapes special characters in a markdown table.
def escape_in_table(desc: str):
  return desc.replace('|', '\\|')


# Responsible for module level markdown generation.
class ModuleMd:

  def __init__(self, module_name: str, module_files: List[Dict[str,
                                                               Any]]) -> None:
    self.module_name = module_name
    self.files_md = sorted([
        FileMd(module_name, file_dict) for file_dict in module_files
    ], key=lambda x: x.import_key)
    self.summary_objs = '\n'.join(
        file.summary_objs for file in self.files_md if file.summary_objs)
    self.summary_funs = '\n'.join(
        file.summary_funs for file in self.files_md if file.summary_funs)
    self.summary_view_funs = '\n'.join(file.summary_view_funs
                                       for file in self.files_md
                                       if file.summary_view_funs)
    self.summary_macros = '\n'.join(
        file.summary_macros for file in self.files_md if file.summary_macros)

  def print_description(self):
    if not self.files_md:
      return ''

    long_s = []
    long_s.append(f'## Module: {self.module_name}')

    if self.module_name == 'prelude':
      # Prelude is a special module which is automatically imported and doesn't
      # have any include keys.
      objs = '\n'.join(obj for file in self.files_md for obj in file.objs)
      if objs:
        long_s.append('#### Views/Tables')
        long_s.append(objs)
      funs = '\n'.join(fun for file in self.files_md for fun in file.funs)
      if funs:
        long_s.append('#### Functions')
        long_s.append(funs)
      table_funs = '\n'.join(
          view_fun for file in self.files_md for view_fun in file.view_funs)
      if table_funs:
        long_s.append('#### Table Functions')
        long_s.append(table_funs)
      macros = '\n'.join(
          macro for file in self.files_md for macro in file.macros)
      if macros:
        long_s.append('#### Macros')
        long_s.append(macros)
      return '\n'.join(long_s)

    for file in self.files_md:
      if not any((file.objs, file.funs, file.view_funs, file.macros)):
        continue

      long_s.append(f'### {file.import_key}')
      if file.objs:
        long_s.append('#### Views/Tables')
        long_s.append('\n'.join(file.objs))
      if file.funs:
        long_s.append('#### Functions')
        long_s.append('\n'.join(file.funs))
      if file.view_funs:
        long_s.append('#### Table Functions')
        long_s.append('\n'.join(file.view_funs))
      if file.macros:
        long_s.append('#### Macros')
        long_s.append('\n'.join(file.macros))

    return '\n'.join(long_s)


# Responsible for file level markdown generation.
class FileMd:

  def __init__(self, module_name, file_dict):
    self.import_key = file_dict['import_key']
    import_key_name = self.import_key if module_name != 'prelude' else 'N/A'
    self.objs, self.funs, self.view_funs, self.macros = [], [], [], []
    summary_objs_list, summary_funs_list, summary_view_funs_list, summary_macros_list = [], [], [], []

    # Add imports if in file.
    for data in file_dict['imports']:
      # Anchor
      anchor = f'''obj/{module_name}/{data['name']}'''

      # Add summary of imported view/table
      summary_objs_list.append(f'''[{data['name']}](#{anchor})|'''
                               f'''{import_key_name}|'''
                               f'''{escape_in_table(data['summary_desc'])}''')

      self.objs.append(f'''\n\n<a name="{anchor}"></a>'''
                       f'''**{data['name']}**, {data['type']}\n\n'''
                       f'''{escape_in_table(data['desc'])}\n''')

      self.objs.append(
          'Column | Type | Description\n------ | --- | -----------')
      for name, info in data['cols'].items():
        self.objs.append(
            f'{name} | {info["type"]} | {escape_in_table(info["desc"])}')

      self.objs.append('\n\n')

    # Add functions if in file
    for data in file_dict['functions']:
      # Anchor
      anchor = f'''fun/{module_name}/{data['name']}'''

      # Add summary of imported function
      summary_funs_list.append(f'''[{data['name']}](#{anchor})|'''
                               f'''{import_key_name}|'''
                               f'''{data['return_type']}|'''
                               f'''{escape_in_table(data['summary_desc'])}''')
      self.funs.append(
          f'''\n\n<a name="{anchor}"></a>'''
          f'''**{data['name']}**\n\n'''
          f'''{data['desc']}\n\n'''
          f'''Returns: {data['return_type']}, {data['return_desc']}\n\n''')
      if data['args']:
        self.funs.append('Argument | Type | Description\n'
                         '-------- | ---- | -----------')
        for name, arg_dict in data['args'].items():
          self.funs.append(
              f'''{name} | {arg_dict['type']} | {escape_in_table(arg_dict['desc'])}'''
          )

        self.funs.append('\n\n')

    # Add table functions if in file
    for data in file_dict['table_functions']:
      # Anchor
      anchor = rf'''view_fun/{module_name}/{data['name']}'''
      # Add summary of imported view function
      summary_view_funs_list.append(
          f'''[{data['name']}](#{anchor})|'''
          f'''{import_key_name}|'''
          f'''{escape_in_table(data['summary_desc'])}''')

      self.view_funs.append(f'''\n\n<a name="{anchor}"></a>'''
                            f'''**{data['name']}**\n'''
                            f'''{data['desc']}\n\n''')
      if data['args']:
        self.view_funs.append('Argument | Type | Description\n'
                              '-------- | ---- | -----------')
        for name, arg_dict in data['args'].items():
          self.view_funs.append(
              f'''{name} | {arg_dict['type']} | {escape_in_table(arg_dict['desc'])}'''
          )
        self.view_funs.append('\n')
      self.view_funs.append('Column | Type | Description\n'
                            '------ | -- | -----------')
      for name, column in data['cols'].items():
        self.view_funs.append(f'{name} | {column["type"]} | {column["desc"]}')

      self.view_funs.append('\n\n')

    # Add macros if in file
    for data in file_dict['macros']:
      # Anchor
      anchor = rf'''macro/{module_name}/{data['name']}'''
      # Add summary of imported view function
      summary_macros_list.append(f'''[{data['name']}](#{anchor})|'''
                                 f'''{import_key_name}|'''
                                 f'''{escape_in_table(data['summary_desc'])}''')

      self.macros.append(
          f'''\n\n<a name="{anchor}"></a>'''
          f'''**{data['name']}**\n'''
          f'''{data['desc']}\n\n'''
          f'''Returns: {data['return_type']}, {data['return_desc']}\n\n''')
      if data['args']:
        self.macros.append('Argument | Type | Description\n'
                           '-------- | ---- | -----------')
        for name, arg_dict in data['args'].items():
          self.macros.append(
              f'''{name} | {arg_dict['type']} | {escape_in_table(arg_dict['desc'])}'''
          )
        self.macros.append('\n')
      self.macros.append('\n\n')

    self.summary_objs = '\n'.join(summary_objs_list)
    self.summary_funs = '\n'.join(summary_funs_list)
    self.summary_view_funs = '\n'.join(summary_view_funs_list)
    self.summary_macros = '\n'.join(summary_macros_list)


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--input', required=True)
  parser.add_argument('--output', required=True)
  args = parser.parse_args()

  with open(args.input) as f:
    modules_json_dict = json.load(f)

  modules_dict: Dict[str, ModuleMd] = {}

  for module_name, module_files in modules_json_dict.items():
    # Remove 'common' when it has been removed from the code.
    if module_name not in ['deprecated', 'common']:
      modules_dict[module_name] = ModuleMd(module_name, module_files)

  prelude_module = modules_dict.pop('prelude')

  with open(args.output, 'w') as f:
    f.write('''
# PerfettoSQL standard library
*This page documents the PerfettoSQL standard library.*

## Introduction
The PerfettoSQL standard library is a repository of tables, views, functions
and macros, contributed by domain experts, which make querying traces easier
Its design is heavily inspired by standard libraries in languages like Python,
C++ and Java.

Some of the purposes of the standard library include:
1) Acting as a way of sharing and commonly written queries without needing
to copy/paste large amounts of SQL.
2) Raising the abstraction level when exposing data in the trace. Many
modules in the standard library convert low-level trace concepts
e.g. slices, tracks and into concepts developers may be more familar with
e.g. for Android developers: app startups, binder transactions etc.

Standard library modules can be included as follows:
```
-- Include all tables/views/functions from the android.startup.startups
-- module in the standard library.
INCLUDE PERFETTO MODULE android.startup.startups;

-- Use the android_startups table defined in the android.startup.startups
-- module.
SELECT *
FROM android_startups;
```

Prelude is a special module is automatically imported. It contains key helper
tables, views and functions which are universally useful.

More information on importing modules is available in the
[syntax documentation](/docs/analysis/perfetto-sql-syntax#including-perfettosql-modules)
for the `INCLUDE PERFETTO MODULE` statement.

<!-- TODO(b/290185551): talk about experimental module and contributions. -->

## Summary
''')

    summary_objs = [prelude_module.summary_objs
                   ] if prelude_module.summary_objs else []
    summary_objs += [
        module.summary_objs
        for name, module in modules_dict.items()
        if (module.summary_objs and name != 'experimental')
    ]

    summary_funs = [prelude_module.summary_funs
                   ] if prelude_module.summary_funs else []
    summary_funs += [
        module.summary_funs
        for name, module in modules_dict.items()
        if (module.summary_funs and name != 'experimental')
    ]
    summary_view_funs = [prelude_module.summary_view_funs
                        ] if prelude_module.summary_view_funs else []
    summary_view_funs += [
        module.summary_view_funs
        for name, module in modules_dict.items()
        if (module.summary_view_funs and name != 'experimental')
    ]
    summary_macros = [prelude_module.summary_macros
                     ] if prelude_module.summary_macros else []
    summary_macros += [
        module.summary_macros
        for name, module in modules_dict.items()
        if (module.summary_macros and name != 'experimental')
    ]

    if summary_objs:
      f.write('### Views/tables\n\n'
              'Name | Import | Description\n'
              '---- | ------ | -----------\n')
      f.write('\n'.join(summary_objs))
      f.write('\n')

    if summary_funs:
      f.write('### Functions\n\n'
              'Name | Import | Return type | Description\n'
              '---- | ------ | ----------- | -----------\n')
      f.write('\n'.join(summary_funs))
      f.write('\n')

    if summary_view_funs:
      f.write('### Table Functions\n\n'
              'Name | Import |  Description\n'
              '---- | ------ |  -----------\n')
      f.write('\n'.join(summary_view_funs))
      f.write('\n')

    if summary_macros:
      f.write('### Macros\n\n'
              'Name | Import |  Description\n'
              '---- | ------ |  -----------\n')
      f.write('\n'.join(summary_macros))
      f.write('\n')

    f.write('\n\n')
    f.write(prelude_module.print_description())
    f.write('\n')
    f.write('\n'.join(
        module.print_description() for module in modules_dict.values()))

  return 0


if __name__ == '__main__':
  sys.exit(main())
