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

from abc import ABC
from dataclasses import dataclass
import re
import sys
from typing import Any, Dict, List, Optional, Set, Tuple, NamedTuple

from python.generators.sql_processing.docs_extractor import DocsExtractor
from python.generators.sql_processing.utils import ObjKind

from python.generators.sql_processing.utils import ALLOWED_PREFIXES
from python.generators.sql_processing.utils import OBJECT_NAME_ALLOWLIST

from python.generators.sql_processing.utils import COLUMN_ANNOTATION_PATTERN
from python.generators.sql_processing.utils import ANY_PATTERN
from python.generators.sql_processing.utils import ARG_DEFINITION_PATTERN
from python.generators.sql_processing.utils import ARG_ANNOTATION_PATTERN


def is_internal(name: str) -> bool:
  return re.match(r'^_.*', name, re.IGNORECASE) is not None


def is_snake_case(s: str) -> bool:
  """Returns true if the string is snake_case."""
  return re.fullmatch(r'^[a-z_0-9]*$', s) is not None


# Parse a SQL comment (i.e. -- Foo\n -- bar.) into a string (i.e. "Foo bar.").
def parse_comment(comment: str) -> str:
  return ' '.join(line.strip().lstrip('--').lstrip()
                  for line in comment.strip().split('\n'))


class Arg(NamedTuple):
  # TODO(b/307926059): the type is missing on old-style documentation for
  # tables. Make it "str" after stdlib is migrated.
  type: Optional[str]
  description: str


# Returns: error message if the name is not correct, None otherwise.
def get_module_prefix_error(name: str, path: str, module: str) -> Optional[str]:
  prefix = name.lower().split('_')[0]
  if module in ["common", "prelude", "deprecated"]:
    if prefix == module:
      return (f'Names of tables/views/functions in the "{module}" module '
              f'should not start with {module}')
    return None
  if prefix == module:
    # Module prefix is always allowed.
    return None
  allowed_prefixes = [module]
  for (path_prefix, allowed_name_prefix) in ALLOWED_PREFIXES.items():
    if path.startswith(path_prefix):
      if prefix == allowed_name_prefix:
        return None
      allowed_prefixes.append(allowed_name_prefix)
    if path in OBJECT_NAME_ALLOWLIST and name in OBJECT_NAME_ALLOWLIST[path]:
      return None
  return (
      f'Names of tables/views/functions at path "{path}" should be prefixed '
      f'with one of following names: {", ".join(allowed_prefixes)}')


class AbstractDocParser(ABC):

  @dataclass
  class Column:
    pass

  def __init__(self, path: str, module: str):
    self.path = path
    self.module = module
    self.name = None
    self.errors = []

  def _parse_name(self, upper: bool = False):
    assert self.name
    assert isinstance(self.name, str)
    module_prefix_error = get_module_prefix_error(self.name, self.path,
                                                  self.module)
    if module_prefix_error is not None:
      self._error(module_prefix_error)
    return self.name.strip()

  def _parse_desc_not_empty(self, desc: str):
    if not desc:
      self._error('Description of the table/view/function/macro is missing')
    return desc.strip()

  def _validate_only_contains_annotations(self,
                                          ans: List[DocsExtractor.Annotation],
                                          ans_types: Set[str]):
    used_ans_types = set(a.key for a in ans)
    for type in used_ans_types.difference(ans_types):
      self._error(f'Unknown documentation annotation {type}')

  def _parse_columns(self, ans: List[DocsExtractor.Annotation],
                     schema: Optional[str]) -> Dict[str, Arg]:
    column_annotations = {}
    for t in ans:
      if t.key != '@column':
        continue
      m = re.match(COLUMN_ANNOTATION_PATTERN, t.value)
      if not m:
        self._error(f'@column annotation value {t.value} does not match '
                    f'pattern {COLUMN_ANNOTATION_PATTERN}')
        continue
      column_annotations[m.group(1)] = Arg(None, m.group(2).strip())

    if not schema:
      # If we don't have schema, we have to accept annotations as the source of
      # truth.
      return column_annotations

    columns = self._parse_args_definition(schema)

    for column in columns:
      inline_comment = columns[column].description
      if not inline_comment and column not in column_annotations:
        self._error(f'Column "{column}" is missing a description. Please add a '
                    'comment in front of the column definition')
        continue

      if column not in column_annotations:
        continue
      annotation = column_annotations[column].description
      if inline_comment and annotation:
        self._error(f'Column "{column}" is documented twice. Please remove the '
                    '@column annotation')
      if not inline_comment and annotation:
        # Absorb old-style annotations.
        columns[column] = Arg(columns[column].type, annotation)

    # Check that the annotations match existing columns.
    for annotation in column_annotations:
      if annotation not in columns:
        self._error(f'Column "{annotation}" is documented but does not exist '
                    'in table definition')
    return columns

  def _parse_args(self, ans: List[DocsExtractor.Annotation],
                  sql_args_str: str) -> Dict[str, Arg]:
    args = self._parse_args_definition(sql_args_str)

    arg_annotations = {}
    for an in ans:
      if an.key != '@arg':
        continue
      m = re.match(ARG_ANNOTATION_PATTERN, an.value)
      if m is None:
        self._error(f'Expected arg documentation "{an.value}" to match pattern '
                    f'{ARG_ANNOTATION_PATTERN}')
        continue
      arg_annotations[m.group(1)] = Arg(m.group(2), m.group(3).strip())

    for arg in args:
      if not args[arg].description and arg not in arg_annotations:
        self._error(f'Arg "{arg}" is missing a description. '
                    'Please add a comment in front of the arg definition.')
      if args[arg].description and arg in arg_annotations:
        self._error(f'Arg "{arg}" is documented twice. '
                    'Please remove the @arg annotation')
      if not args[arg].description and arg in arg_annotations:
        # Absorb old-style annotations.
        # TODO(b/307926059): Remove it once stdlib is migrated.
        args[arg] = Arg(args[arg].type, arg_annotations[arg].description)

    for arg in arg_annotations:
      if arg not in args:
        self._error(
            f'Arg "{arg}" is documented but not found in function definition.')
    return args

  # Parse function argument definition list or a table schema, e.g.
  # arg1 INT, arg2 STRING, including their comments.
  def _parse_args_definition(self, args_str: str) -> Dict[str, Arg]:
    result = {}
    remaining_args = args_str.strip()
    while remaining_args:
      m = re.match(fr'^{ARG_DEFINITION_PATTERN}({ANY_PATTERN})', remaining_args)
      if not m:
        self._error(f'Expected "{args_str}" to correspond to '
                    '"-- Comment\n arg_name TYPE" format '
                    '({ARG_DEFINITION_PATTERN})')
        return result
      groups = m.groups()
      comment = '' if groups[0] is None else parse_comment(groups[0])
      name = groups[-3]
      type = groups[-2]
      result[name] = Arg(type, comment)
      # Strip whitespace and comma and parse the next arg.
      remaining_args = groups[-1].lstrip().lstrip(',').lstrip()

    return result

  def _error(self, error: str):
    self.errors.append(
        f'Error while parsing documentation for "{self.name}" in {self.path}: '
        f'{error}')


class TableOrView:
  name: str
  type: str
  desc: str
  cols: Dict[str, Arg]

  def __init__(self, name, type, desc, cols):
    self.name = name
    self.type = type
    self.desc = desc
    self.cols = cols


class TableViewDocParser(AbstractDocParser):
  """Parses documentation for CREATE TABLE and CREATE VIEW statements."""

  def __init__(self, path: str, module: str):
    super().__init__(path, module)

  def parse(self, doc: DocsExtractor.Extract) -> Optional[TableOrView]:
    assert doc.obj_kind == ObjKind.table_view

    or_replace, perfetto_or_virtual, type, self.name, schema = doc.obj_match

    if or_replace is not None:
      self._error(
          f'{type} "{self.name}": CREATE OR REPLACE is not allowed in stdlib '
          f'as standard library modules can only included once. Please just '
          f'use CREATE instead.')
    if is_internal(self.name):
      return None

    is_perfetto_table_or_view = (
        perfetto_or_virtual and perfetto_or_virtual.lower() == 'perfetto')
    if not schema and is_perfetto_table_or_view:
      self._error(
          f'{type} "{self.name}": schema is missing for a non-internal stdlib'
          f' perfetto table or view')

    self._validate_only_contains_annotations(doc.annotations, {'@column'})
    return TableOrView(
        name=self._parse_name(),
        type=type,
        desc=self._parse_desc_not_empty(doc.description),
        cols=self._parse_columns(doc.annotations, schema),
    )


class Function:
  name: str
  desc: str
  args: Dict[str, Arg]
  return_type: str
  return_desc: str

  def __init__(self, name, desc, args, return_type, return_desc):
    self.name = name
    self.desc = desc
    self.args = args
    self.return_type = return_type
    self.return_desc = return_desc


class FunctionDocParser(AbstractDocParser):
  """Parses documentation for CREATE_FUNCTION statements."""

  def __init__(self, path: str, module: str):
    super().__init__(path, module)

  def parse(self, doc: DocsExtractor.Extract) -> Optional[Function]:
    or_replace, self.name, args, ret_comment, ret_type = doc.obj_match

    if or_replace is not None:
      self._error(
          f'Function "{self.name}": CREATE OR REPLACE is not allowed in stdlib '
          f'as standard library modules can only included once. Please just '
          f'use CREATE instead.')

    # Ignore internal functions.
    if is_internal(self.name):
      return None

    name = self._parse_name()

    if not is_snake_case(name):
      self._error(f'Function name "{name}" is not snake_case'
                  f' (should be {name.casefold()})')

    ret_desc = None if ret_comment is None else parse_comment(ret_comment)
    if not ret_desc:
      self._error(f'Function "{name}": return description is missing')

    return Function(
        name=name,
        desc=self._parse_desc_not_empty(doc.description),
        args=self._parse_args(doc.annotations, args),
        return_type=ret_type,
        return_desc=ret_desc,
    )


class TableFunction:
  name: str
  desc: str
  cols: Dict[str, Arg]
  args: Dict[str, Arg]

  def __init__(self, name, desc, cols, args):
    self.name = name
    self.desc = desc
    self.cols = cols
    self.args = args


class TableFunctionDocParser(AbstractDocParser):
  """Parses documentation for table function statements."""

  def __init__(self, path: str, module: str):
    super().__init__(path, module)

  def parse(self, doc: DocsExtractor.Extract) -> Optional[TableFunction]:
    or_replace, self.name, args, ret_comment, columns = doc.obj_match

    if or_replace is not None:
      self._error(
          f'Function "{self.name}": CREATE OR REPLACE is not allowed in stdlib '
          f'as standard library modules can only included once. Please just '
          f'use CREATE instead.')

    # Ignore internal functions.
    if is_internal(self.name):
      return None

    self._validate_only_contains_annotations(doc.annotations,
                                             {'@arg', '@column'})
    name = self._parse_name()

    if not is_snake_case(name):
      self._error(f'Function name "{name}" is not snake_case'
                  f' (should be "{name.casefold()}")')

    return TableFunction(
        name=name,
        desc=self._parse_desc_not_empty(doc.description),
        cols=self._parse_columns(doc.annotations, columns),
        args=self._parse_args(doc.annotations, args),
    )


class Macro:
  name: str
  desc: str
  return_desc: str
  return_type: str
  args: Dict[str, Arg]

  def __init__(self, name: str, desc: str, return_desc: str, return_type: str,
               args: Dict[str, Arg]):
    self.name = name
    self.desc = desc
    self.return_desc = return_desc
    self.return_type = return_type
    self.args = args


class MacroDocParser(AbstractDocParser):
  """Parses documentation for macro statements."""

  def __init__(self, path: str, module: str):
    super().__init__(path, module)

  def parse(self, doc: DocsExtractor.Extract) -> Optional[Macro]:
    or_replace, self.name, args, return_desc, return_type = doc.obj_match

    if or_replace is not None:
      self._error(
          f'Function "{self.name}": CREATE OR REPLACE is not allowed in stdlib '
          f'as standard library modules can only included once. Please just '
          f'use CREATE instead.')

    # Ignore internal macros.
    if is_internal(self.name):
      return None

    self._validate_only_contains_annotations(doc.annotations, set())
    name = self._parse_name()

    if not is_snake_case(name):
      self._error(f'Macro name "{name}" is not snake_case'
                  f' (should be "{name.casefold()}")')

    return Macro(
        name=name,
        desc=self._parse_desc_not_empty(doc.description),
        return_desc=parse_comment(return_desc),
        return_type=return_type,
        args=self._parse_args(doc.annotations, args),
    )


class ParsedFile:
  errors: List[str] = []
  table_views: List[TableOrView] = []
  functions: List[Function] = []
  table_functions: List[TableFunction] = []
  macros: List[Macro] = []

  def __init__(self, errors: List[str], table_views: List[TableOrView],
               functions: List[Function], table_functions: List[TableFunction],
               macros: List[Macro]):
    self.errors = errors
    self.table_views = table_views
    self.functions = functions
    self.table_functions = table_functions
    self.macros = macros


# Reads the provided SQL and, if possible, generates a dictionary with data
# from documentation together with errors from validation of the schema.
def parse_file(path: str, sql: str) -> Optional[ParsedFile]:
  if sys.platform.startswith('win'):
    path = path.replace('\\', '/')

  # Get module name
  module_name = path.split('/stdlib/')[-1].split('/')[0]

  # Disable support for `deprecated` module
  if module_name == "deprecated":
    return

  # Extract all the docs from the SQL.
  extractor = DocsExtractor(path, module_name, sql)
  docs = extractor.extract()
  if extractor.errors:
    return ParsedFile(extractor.errors, [], [], [], [])

  # Parse the extracted docs.
  errors = []
  table_views = []
  functions = []
  table_functions = []
  macros = []
  for doc in docs:
    if doc.obj_kind == ObjKind.table_view:
      parser = TableViewDocParser(path, module_name)
      res = parser.parse(doc)
      if res:
        table_views.append(res)
      errors += parser.errors
    if doc.obj_kind == ObjKind.function:
      parser = FunctionDocParser(path, module_name)
      res = parser.parse(doc)
      if res:
        functions.append(res)
      errors += parser.errors
    if doc.obj_kind == ObjKind.table_function:
      parser = TableFunctionDocParser(path, module_name)
      res = parser.parse(doc)
      if res:
        table_functions.append(res)
      errors += parser.errors
    if doc.obj_kind == ObjKind.macro:
      parser = MacroDocParser(path, module_name)
      res = parser.parse(doc)
      if res:
        macros.append(res)
      errors += parser.errors

  return ParsedFile(errors, table_views, functions, table_functions, macros)
