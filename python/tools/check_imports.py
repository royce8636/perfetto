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
"""
Enforce import rules for https://ui.perfetto.dev.
Directory structure encodes ideas about the expected dependency graph
of the code in those directories. Both in a fuzzy sense: we expect code
withing a directory to have high cohesion within the directory and low
coupling (aka fewer imports) outside of the directory - but also
concrete rules:
- "base should not depend on the fronted"
- "plugins should only directly depend on the public API"
- "we should not have circular dependencies"

Without enforcement exceptions to this rule quickly slip in. This
script allows such rules to be enforced at presubmit time.
"""

import sys
import os
import re
import collections
import argparse

ROOT_DIR = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
UI_SRC_DIR = os.path.join(ROOT_DIR, 'ui', 'src')


class Failure(object):

  def __init__(self, path, rule):
    self.path = path
    self.rule = rule

  def __str__(self):
    nice_path = ["ui/src" + name + ".ts" for name in self.path]
    return ''.join([
        'Forbidden dependency path:\n\n ',
        '\n    -> '.join(nice_path),
        '\n',
        '\n',
        str(self.rule),
        '\n',
    ])


class NoDirectDep(object):

  def __init__(self, src, dst, reasoning):
    self.src = src
    self.dst = dst
    self.reasoning = reasoning

  def check(self, graph):
    for node, edges in graph.items():
      if re.match(self.src, node):
        for edge in edges:
          if re.match(self.dst, edge):
            yield Failure([node, edge], self)

  def __str__(self):
    return f'"{self.src}" may not directly depend on "{self.dst}" ' + self.reasoning


class NoDep(object):

  def __init__(self, src, dst, reasoning):
    self.src = src
    self.dst = dst
    self.reasoning = reasoning

  def check(self, graph):
    for node in graph:
      if re.match(self.src, node):
        for connected, path in bfs(graph, node):
          if re.match(self.dst, connected):
            yield Failure(path, self)

  def __str__(self):
    return f'"{self.src}" may not depend on "{self.dst}" ' + self.reasoning


class NoCircularDeps(object):

  def __init__(self):
    pass

  def check(self, graph):
    for node in graph:
      for child in graph[node]:
        for reached, path in dfs(graph, child):
          if reached == node:
            yield Failure([node] + path, self)

  def __str__(self):
    return f'circular dependencies can cause complex issues'


# We have three kinds of rules:
# NoDirectDep(a, b) = files matching regex 'a' cannot *directly* import
#   files matching regex 'b' - but they may indirectly depend on them.
# NoDep(a, b) = as above but 'a' may not even transitively import 'b'.
# NoCircularDeps = forbid introduction of circular dependencies
RULES = [
    NoDirectDep(
        r'/plugins/.*',
        r'/core/.*',
        'instead plugins should depend on the API exposed at ui/src/public.',
    ),
    #NoDirectDep(
    #    r'/tracks/.*',
    #    r'/core/.*',
    #    'instead tracks should depend on the API exposed at ui/src/public.',
    #),
    NoDep(
        r'/core/.*',
        r'/plugins/.*',
        'otherwise the plugins are no longer optional.',
    ),
    NoDep(
        r'/core/.*',
        r'/frontend/.*',
        'trying to reduce the dependency mess as we refactor into core',
    ),
    NoDep(
        r'/core/.*',
        r'/common/.*',
        'trying to reduce the dependency mess as we refactor into core',
    ),
    NoDep(
        r'/core/.*',
        r'/controller/.*',
        'trying to reduce the dependency mess as we refactor into core',
    ),

    # Fails at the moment due to:
    # ui/src/base/comparison_utils.ts
    #    -> ui/src/common/query_result.ts
    #    -> ui/src/core/static_initializers.ts
    #NoDep(
    #  r'/base/.*',
    #  r'/core/.*',
    #  'core should depend on base not the other way round',
    #),

    # Fails at the moment as we have several circular dependencies. One
    # example:
    # ui/src/frontend/cookie_consent.ts
    #    -> ui/src/frontend/globals.ts
    #    -> ui/src/frontend/router.ts
    #    -> ui/src/frontend/pages.ts
    #    -> ui/src/frontend/cookie_consent.ts
    #NoCircularDeps(),
]


def all_source_files():
  for root, dirs, files in os.walk(UI_SRC_DIR, followlinks=False):
    for name in files:
      if name.endswith('.ts') and not name.endswith('.d.ts'):
        yield os.path.join(root, name)


def is_dir(path, cache={}):
  try:
    return cache[path]
  except KeyError:
    result = cache[path] = os.path.isdir(path)
    return result


def find_imports(path):
  src = path
  src = src.removeprefix(UI_SRC_DIR)
  src = src.removesuffix('.ts')
  directory, _ = os.path.split(src)
  with open(path) as f:
    s = f.read()
    for m in re.finditer("^import[^']*'([^']*)';", s, flags=re.MULTILINE):
      raw_target = m[1]
      if raw_target.startswith('.'):
        target = os.path.normpath(os.path.join(directory, raw_target))
        if is_dir(UI_SRC_DIR + target):
          target = os.path.join(target, 'index')
      else:
        target = raw_target
      yield (src, target)


def path_to_id(path):
  return path.replace('/', '_').replace('-', '_').replace('@', '_at_')


def is_external_dep(path):
  return not path.startswith('/')


def bfs(graph, src):
  seen = set()
  queue = [(src, [])]

  while queue:
    node, path = queue.pop(0)
    if node in seen:
      continue

    seen.add(node)

    path = path[:]
    path.append(node)

    yield node, path
    queue.extend([(child, path) for child in graph[node]])


def dfs(graph, src):
  seen = set()
  queue = [(src, [])]

  while queue:
    node, path = queue.pop()
    if node in seen:
      continue

    seen.add(node)

    path = path[:]
    path.append(node)

    yield node, path
    queue.extend([(child, path) for child in graph[node]])


def write_dot(graph, f):
  print('digraph g {', file=f)
  for node, edges in graph.items():
    node_id = path_to_id(node)
    shape = 'rectangle' if is_external_dep(node) else 'ellipse'
    print(f'{node_id} [shape={shape}, label="{node}"];', file=f)

    for edge in edges:
      edge_id = path_to_id(edge)
      print(f'{node_id} -> {edge_id};', file=f)
  print('}', file=f)


def do_check(options, graph):
  for rule in RULES:
    for failure in rule.check(graph):
      print(failure)
      return 1
  return 0


def do_desc(options, graph):
  print('Rules:')
  for rule in RULES:
    print("  - ", end='')
    print(rule)


def do_dot(options, graph):

  def simplify(path):
    if is_external_dep(path):
      return path
    return os.path.dirname(path)

  if options.simplify:
    new_graph = collections.defaultdict(set)
    for node, edges in graph.items():
      for edge in edges:
        new_graph[simplify(edge)]
        new_graph[simplify(node)].add(simplify(edge))
    graph = new_graph

  if options.ignore_external:
    new_graph = collections.defaultdict(set)
    for node, edges in graph.items():
      if is_external_dep(node):
        continue
      for edge in edges:
        if is_external_dep(edge):
          continue
        new_graph[edge]
        new_graph[node].add(edge)
    graph = new_graph

  write_dot(graph, sys.stdout)
  return 0


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.set_defaults(func=do_check)
  subparsers = parser.add_subparsers()

  check_command = subparsers.add_parser(
      'check', help='Check the rules (default)')
  check_command.set_defaults(func=do_check)

  desc_command = subparsers.add_parser('desc', help='Print the rules')
  desc_command.set_defaults(func=do_desc)

  dot_command = subparsers.add_parser(
      'dot',
      help='Output dependency graph in dot format suitble for use in graphviz (e.g. ./tools/check_imports dot | dot -Tpng -ograph.png)'
  )
  dot_command.set_defaults(func=do_dot)
  dot_command.add_argument(
      '--simplify',
      action='store_true',
      help='Show directories rather than files',
  )
  dot_command.add_argument(
      '--ignore-external',
      action='store_true',
      help='Don\'t show external dependencies',
  )

  graph = collections.defaultdict(set)
  for path in all_source_files():
    for src, target in find_imports(path):
      graph[src].add(target)
      graph[target]

  options = parser.parse_args()
  return options.func(options, graph)


if __name__ == '__main__':
  sys.exit(main())
