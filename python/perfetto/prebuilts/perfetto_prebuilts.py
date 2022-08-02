# Copyright (C) 2021 The Android Open Source Project
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
Functions to fetch pre-pinned Perfetto prebuilts.

This function is used in different places:
- Into the //tools/{trace_processor, traceconv} scripts, which are just plain
  wrappers around executables.
- Into the //tools/{heap_profiler, record_android_trace} scripts, which contain
  some other hand-written python code.

The manifest argument looks as follows:
TRACECONV_MANIFEST = [
  {
    'arch': 'mac-amd64',
    'file_name': 'traceconv',
    'file_size': 7087080,
    'url': https://commondatastorage.googleapis.com/.../trace_to_text',
    'sha256': 7d957c005b0dc130f5bd855d6cec27e060d38841b320d04840afc569f9087490',
    'platform': 'darwin',
    'machine': 'x86_64'
  },
  ...
]

The intended usage is:

  from perfetto.prebuilts.manifests.traceconv import TRACECONV_MANIFEST
  bin_path = get_perfetto_prebuilt(TRACECONV_MANIFEST)
  subprocess.call(bin_path, ...)
"""

import hashlib
import os
import platform
import subprocess
import sys


def download_or_get_cached(file_name, url, sha256):
  """ Downloads a prebuilt or returns a cached version

  The first time this is invoked, it downloads the |url| and caches it into
  ~/.perfetto/prebuilts/$tool_name. On subsequent invocations it just runs the
  cached version.
  """
  dir = os.path.join(
      os.path.expanduser('~'), '.local', 'share', 'perfetto', 'prebuilts')
  os.makedirs(dir, exist_ok=True)
  bin_path = os.path.join(dir, file_name)
  sha256_path = os.path.join(dir, file_name + '.sha256')
  needs_download = True

  # Avoid recomputing the SHA-256 on each invocation. The SHA-256 of the last
  # download is cached into file_name.sha256, just check if that matches.
  if os.path.exists(bin_path) and os.path.exists(sha256_path):
    with open(sha256_path, 'rb') as f:
      digest = f.read().decode()
      if digest == sha256:
        needs_download = False

  if needs_download:
    # Either the filed doesn't exist or the SHA256 doesn't match.
    tmp_path = bin_path + '.tmp'
    print('Downloading ' + url)
    subprocess.check_call(['curl', '-f', '-L', '-#', '-o', tmp_path, url])
    with open(tmp_path, 'rb') as fd:
      actual_sha256 = hashlib.sha256(fd.read()).hexdigest()
    if actual_sha256 != sha256:
      raise Exception('Checksum mismatch for %s (actual: %s, expected: %s)' %
                      (url, actual_sha256, sha256))
    os.chmod(tmp_path, 0o755)
    os.rename(tmp_path, bin_path)
    with open(sha256_path, 'w') as f:
      f.write(sha256)
  return bin_path


def get_perfetto_prebuilt(manifest, soft_fail=False, arch=None):
  """ Downloads the prebuilt, if necessary, and returns its path on disk. """
  plat = sys.platform.lower()
  machine = platform.machine().lower()
  manifest_entry = None
  for entry in manifest:
    # If the caller overrides the arch, just match that (for Android prebuilts).
    if arch:
      if entry.get('arch') == arch:
        manifest_entry = entry
        break
      continue
    # Otherwise guess the local machine arch.
    if entry.get('platform') == plat and machine in entry.get('machine', []):
      manifest_entry = entry
      break
  if manifest_entry is None:
    if soft_fail:
      return None
    raise Exception(
        ('No prebuilts available for %s-%s\n' % (plat, machine)) +
        'See https://perfetto.dev/docs/contributing/build-instructions')

  return download_or_get_cached(
      file_name=manifest_entry['file_name'],
      url=manifest_entry['url'],
      sha256=manifest_entry['sha256'])


def run_perfetto_prebuilt(manifest):
  bin_path = get_perfetto_prebuilt(manifest)
  if sys.platform.lower() == 'win32':
    sys.exit(subprocess.check_call([bin_path, *sys.argv[1:]]))
  os.execv(bin_path, [bin_path] + sys.argv[1:])
