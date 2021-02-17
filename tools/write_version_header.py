#!/usr/bin/env python
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
"""
Writes the perfetto_version{.gen.h, .ts} files.

This tool is run as part of a genrule from GN, SoonG and Bazel builds. It
generates a source header (or in the case of --ts_out a TypeScript file) that
contains:
- The version number (e.g. v9.0) obtained parsing the CHANGELOG file.
- The git HEAD's commit-ish (e.g. 6b330b772b0e973f79c70ba2e9bb2b0110c6715d)
- The number of CLs from the release tag to HEAD.

The latter is concatenated to the version number to distinguish builds made
fully from release tags (e.g., v9.0.0) vs builds made from the main branch which
are N cls ahead of the latest monthly release (e.g., v9.0.42).
"""

import argparse
import os
import re
import sys
import subprocess

# Note: PROJECT_ROOT is not accurate in bazel builds, where this script is
# executed in the bazel sandbox.
PROJECT_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SCM_REV_NOT_AVAILABLE = 'N/A'


def get_latest_release(changelog_path):
  """Returns a string like 'v9.0'.

  It does so by searching the latest version mentioned in the CHANGELOG."""
  if not changelog_path:
    if os.path.exists('CHANGELOG'):
      changelog_path = 'CHANGELOG'
    else:
      changelog_path = os.path.join(PROJECT_ROOT, 'CHANGELOG')
  with open(changelog_path) as f:
    for line in f.readlines():
      m = re.match('^(v\d+[.]\d+)\s.*$', line)
      if m is not None:
        return m.group(1)
  raise Exception('Failed to fetch Perfetto version from %s' % changelog_path)


def get_git_info(last_release_tag):
  """Returns a tuple ('deadbeef', '1234').

  The first value is the SHA1 of the HEAD. The second is the number of CLs from
  the passed |last_release_tag| to HEAD."""
  commit_sha1 = SCM_REV_NOT_AVAILABLE
  commits_since_release = ''
  git_dir = os.path.join(PROJECT_ROOT, '.git')
  if os.path.exists(git_dir):
    try:
      commit_sha1 = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                            cwd=PROJECT_ROOT).strip().decode()
      with open(os.devnull, 'wb') as devnull:
        commits_since_release = subprocess.check_output(
            [
                'git', 'rev-list', '--count',
                'refs/tags/%s..HEAD' % last_release_tag
            ],
            cwd=PROJECT_ROOT,
            stderr=devnull).strip().decode()
    except subprocess.CalledProcessError:
      pass

  return (commit_sha1, commits_since_release)


def write_if_unchanged(path, content):
  prev_content = None
  if os.path.exists(path):
    with open(path, 'r') as fprev:
      prev_content = fprev.read()
  if prev_content == content:
    return 0
  with open(path, 'w') as fout:
    fout.write(content)


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--no_git',
      action='store_true',
      help='Skips running git rev-parse, emits only the version from CHANGELOG')
  parser.add_argument('--cpp_out', help='Path of the generated .h file.')
  parser.add_argument('--ts_out', help='Path of the generated .ts file.')
  parser.add_argument('--stdout', help='Write to stdout', action='store_true')
  parser.add_argument('--changelog', help='Path to CHANGELOG.')
  args = parser.parse_args()

  release = get_latest_release(args.changelog)
  if args.no_git:
    git_sha1, commits_since_release = (SCM_REV_NOT_AVAILABLE, '')
  else:
    git_sha1, commits_since_release = get_git_info(release)

  # Try to compute the number of commits since the last release. This can fail
  # in some environments (e.g. in android builds) because the bots pull only
  # the main branch and don't pull the whole list of tags.
  if commits_since_release:
    version = '%s.%s' % (release, commits_since_release)  # e.g., 'v9.0.42'.
  else:
    version = release  # e.g., 'v9.0'.

  if args.cpp_out:
    guard = '%s_' % args.cpp_out.upper()
    guard = re.sub(r'[^\w]', '_', guard)
    lines = []
    lines.append('// Generated by %s' % __file__)
    lines.append('')
    lines.append('#ifndef %s' % guard)
    lines.append('#define %s' % guard)
    lines.append('')
    lines.append('#define PERFETTO_VERSION_STRING() "%s"' % version)
    lines.append('#define PERFETTO_VERSION_SCM_REVISION() "%s"' % git_sha1)
    lines.append('')
    lines.append('#endif  // %s' % guard)
    lines.append('')
    content = '\n'.join(lines)
    write_if_unchanged(args.cpp_out, content)

  if args.ts_out:
    lines = []
    lines.append('export const VERSION = "%s";' % version)
    lines.append('export const SCM_REVISION = "%s";' % git_sha1)
    content = '\n'.join(lines)
    write_if_unchanged(args.ts_out, content)

  if args.stdout:
    print(version)


if __name__ == '__main__':
  sys.exit(main())
