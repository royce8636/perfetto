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

import concurrent.futures
import datetime
import difflib
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from typing import Dict, List, Tuple

from google.protobuf import text_format
from python.generators.diff_tests.testing import DiffTest, TestType
from python.generators.diff_tests.utils import (
    ColorFormatter, create_message_factory, get_env, get_trace_descriptor_path,
    read_all_tests, serialize_python_trace, serialize_textproto_trace)

ROOT_DIR = os.path.dirname(
    os.path.dirname(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


# Performance result of running the test.
@dataclass
class PerfResult:
  test_type: TestType
  trace_path: str
  query_path_or_metric: str
  ingest_time_ns: int
  real_time_ns: int

  def __init__(self, test: DiffTest, perf_lines: List[str]):
    self.test_type = test.type
    self.trace_path = test.trace_path
    self.query_path_or_metric = test.query_path

    assert len(perf_lines) == 1
    perf_numbers = perf_lines[0].split(',')

    assert len(perf_numbers) == 2
    self.ingest_time_ns = int(perf_numbers[0])
    self.real_time_ns = int(perf_numbers[1])


# Data gathered from running the test.
@dataclass
class TestResult:
  test: DiffTest
  trace: str
  cmd: List[str]
  expected: str
  actual: str
  passed: bool
  stderr: str
  exit_code: int
  perf_result: PerfResult

  def __init__(self, test: DiffTest, gen_trace_path: str, cmd: List[str],
               expected_text: str, actual_text: str, stderr: str,
               exit_code: int, perf_lines: List[str]) -> None:
    self.test = test
    self.trace = gen_trace_path
    self.cmd = cmd
    self.stderr = stderr
    self.exit_code = exit_code

    # For better string formatting we often add whitespaces, which has to now
    # be removed.
    self.expected = expected_text.lstrip('\n')
    self.actual = actual_text.lstrip('\n')

    expected_content = self.expected.replace('\r\n', '\n')
    actual_content = self.actual.replace('\r\n', '\n')
    self.passed = (expected_content == actual_content)

    self.perf_result = PerfResult(self.test, perf_lines)

  def write_diff(self):
    expected_lines = self.expected.splitlines(True)
    actual_lines = self.actual.splitlines(True)
    diff = difflib.unified_diff(
        expected_lines, actual_lines, fromfile='expected', tofile='actual')
    return "".join(list(diff))

  def rebase(self, rebase) -> str:
    if not rebase or self.passed:
      return ""
    if not self.test.blueprint.is_out_file():
      return f"Can't rebase expected results passed as strings.\n"
    if self.exit_code != 0:
      return f"Rebase failed for {self.test.name} as query failed\n"

    with open(self.test.expected_path, 'w') as f:
      f.write(self.actual)
    return f"Rebasing {self.test.name}\n"


# Results of running the test suite. Mostly used for printing aggregated
# results.
@dataclass
class TestResults:
  test_failures: List[str]
  perf_data: List[PerfResult]
  rebased: List[str]
  test_time_ms: int

  def str(self, no_colors: bool, tests_no: int):
    c = ColorFormatter(no_colors)
    res = (
        f"[==========] {tests_no} tests ran. ({self.test_time_ms} ms total)\n"
        f"{c.green('[  PASSED  ]')} "
        f"{tests_no - len(self.test_failures)} tests.\n")
    if len(self.test_failures) > 0:
      res += (f"{c.red('[  FAILED  ]')} " f"{len(self.test_failures)} tests.\n")
      for failure in self.test_failures:
        res += f"{c.red('[  FAILED  ]')} {failure}\n"
    return res

  def rebase_str(self):
    res = f"\n[  REBASED  ] {len(self.rebased)} tests.\n"
    for name in self.rebased:
      res += f"[  REBASED  ] {name}\n"
    return res


# Responsible for executing singular diff test.
@dataclass
class DiffTestExecutor:
  test: DiffTest
  trace_processor_path: str
  trace_descriptor_path: str
  colors: ColorFormatter

  def __run_metrics_test(self, gen_trace_path: str,
                         metrics_message_factory) -> TestResult:
    if self.test.blueprint.is_out_file():
      with open(self.test.expected_path, 'r') as expected_file:
        expected = expected_file.read()
    else:
      expected = self.test.blueprint.out.contents

    tmp_perf_file = tempfile.NamedTemporaryFile(delete=False)
    is_json_output_file = self.test.blueprint.is_out_file(
    ) and os.path.basename(self.test.expected_path).endswith('.json.out')
    is_json_output = is_json_output_file or self.test.blueprint.is_out_json()
    cmd = [
        self.trace_processor_path,
        '--analyze-trace-proto-content',
        '--crop-track-events',
        '--run-metrics',
        self.test.blueprint.query.name,
        '--metrics-output=%s' % ('json' if is_json_output else 'binary'),
        '--perf-file',
        tmp_perf_file.name,
        gen_trace_path,
    ]
    tp = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=get_env(ROOT_DIR))
    (stdout, stderr) = tp.communicate()

    if is_json_output:
      expected_text = expected
      actual_text = stdout.decode('utf8')
    else:
      # Expected will be in text proto format and we'll need to parse it to
      # a real proto.
      expected_message = metrics_message_factory()
      text_format.Merge(expected, expected_message)

      # Actual will be the raw bytes of the proto and we'll need to parse it
      # into a message.
      actual_message = metrics_message_factory()
      actual_message.ParseFromString(stdout)

      # Convert both back to text format.
      expected_text = text_format.MessageToString(expected_message)
      actual_text = text_format.MessageToString(actual_message)

    perf_lines = [line.decode('utf8') for line in tmp_perf_file.readlines()]
    tmp_perf_file.close()
    os.remove(tmp_perf_file.name)
    return TestResult(self.test,
                      gen_trace_path, cmd, expected_text, actual_text,
                      stderr.decode('utf8'), tp.returncode, perf_lines)

  # Run a query based Diff Test.
  def __run_query_test(self, gen_trace_path: str) -> TestResult:
    if self.test.expected_path:
      with open(self.test.expected_path, 'r') as expected_file:
        expected = expected_file.read()
    else:
      expected = self.test.blueprint.out.contents
    tmp_perf_file = tempfile.NamedTemporaryFile(delete=False)
    cmd = [
        self.trace_processor_path,
        '--analyze-trace-proto-content',
        '--crop-track-events',
        '-q',
        self.test.query_path,
        '--perf-file',
        tmp_perf_file.name,
        gen_trace_path,
    ]
    tp = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=get_env(ROOT_DIR))
    (stdout, stderr) = tp.communicate()

    perf_lines = [line.decode('utf8') for line in tmp_perf_file.readlines()]
    tmp_perf_file.close()
    os.remove(tmp_perf_file.name)

    return TestResult(self.test, gen_trace_path, cmd, expected,
                      stdout.decode('utf8'), stderr.decode('utf8'),
                      tp.returncode, perf_lines)

  def __run(self, metrics_descriptor_paths: List[str],
            extension_descriptor_paths: List[str], keep_input,
            rebase) -> Tuple[TestResult, str]:
    is_generated_trace = True
    if self.test.trace_path.endswith('.py'):
      gen_trace_file = tempfile.NamedTemporaryFile(delete=False)
      serialize_python_trace(ROOT_DIR, self.trace_descriptor_path,
                             self.test.trace_path, gen_trace_file)
      gen_trace_path = os.path.realpath(gen_trace_file.name)

    elif self.test.trace_path.endswith('.textproto'):
      gen_trace_file = tempfile.NamedTemporaryFile(delete=False)
      serialize_textproto_trace(self.trace_descriptor_path,
                                extension_descriptor_paths,
                                self.test.trace_path, gen_trace_file)
      gen_trace_path = os.path.realpath(gen_trace_file.name)

    else:
      gen_trace_file = None
      gen_trace_path = self.test.trace_path
      is_generated_trace = False

    str = f"{self.colors.yellow('[ RUN      ]')} {self.test.name}\n"

    if self.test.type == TestType.QUERY:
      if not os.path.exists(self.test.query_path):
        return None, str + f"Query file not found {self.test.query_path}"

      result = self.__run_query_test(gen_trace_path)
    elif self.test.type == TestType.METRIC:
      result = self.__run_metrics_test(
          gen_trace_path,
          create_message_factory(metrics_descriptor_paths,
                                 'perfetto.protos.TraceMetrics'))
    else:
      assert False

    if gen_trace_file:
      if keep_input:
        str += f"Saving generated input trace: {gen_trace_path}\n"
      else:
        gen_trace_file.close()
        os.remove(gen_trace_path)

    def write_cmdlines():
      res = ""
      if is_generated_trace:
        res += 'Command to generate trace:\n'
        res += 'tools/serialize_test_trace.py '
        res += '--descriptor {} {} > {}\n'.format(
            os.path.relpath(self.trace_descriptor_path, ROOT_DIR),
            os.path.relpath(self.test.trace_path, ROOT_DIR),
            os.path.relpath(gen_trace_path, ROOT_DIR))
      res += f"Command line:\n{' '.join(result.cmd)}\n"
      return res

    if result.exit_code != 0 or not result.passed:
      str += result.stderr

      if result.exit_code == 0:
        str += f"Expected did not match actual for test {self.test.name}.\n"
        str += write_cmdlines()
        str += result.write_diff()
      else:
        str += write_cmdlines()

      str += (f"{self.colors.red('[  FAILED  ]')} {self.test.name}\n")
      str += result.rebase(rebase)

      return result, str
    else:
      str += (f"{self.colors.green('[       OK ]')} {self.test.name} "
              f"(ingest: {result.perf_result.ingest_time_ns / 1000000:.2f} ms "
              f"query: {result.perf_result.real_time_ns / 1000000:.2f} ms)\n")
    return result, str

  # Run a DiffTest.
  def execute(self, extension_descriptor_paths: List[str],
              metrics_descriptor: str, keep_input: bool,
              rebase: bool) -> Tuple[str, str, TestResult]:
    if metrics_descriptor:
      metrics_descriptor_paths = [metrics_descriptor]
    else:
      out_path = os.path.dirname(self.trace_processor_path)
      metrics_protos_path = os.path.join(out_path, 'gen', 'protos', 'perfetto',
                                         'metrics')
      metrics_descriptor_paths = [
          os.path.join(metrics_protos_path, 'metrics.descriptor'),
          os.path.join(metrics_protos_path, 'chrome',
                       'all_chrome_metrics.descriptor')
      ]
    result_str = ""

    if not os.path.exists(self.test.trace_path):
      result_str += f"Trace file not found {self.test.trace_path}\n"
      return self.test.name, result_str, None
    elif self.test.expected_path and not os.path.exists(
        self.test.expected_path):
      result_str = f"Expected file not found {self.test.expected_path}"
      return self.test.name, result_str, None

    # We can't use delete=True here. When using that on Windows, the
    # resulting file is opened in exclusive mode (in turn that's a subtle
    # side-effect of the underlying CreateFile(FILE_ATTRIBUTE_TEMPORARY))
    # and TP fails to open the passed path.
    result, run_str = self.__run(metrics_descriptor_paths,
                                 extension_descriptor_paths, keep_input, rebase)
    result_str += run_str
    if not result:
      return self.test.name, result_str, None

    return self.test.name, result_str, result


# Fetches and executes all diff viable tests.
@dataclass
class DiffTestSuiteRunner:
  tests: List[DiffTest]
  trace_processor_path: str
  trace_descriptor_path: str
  test_runners: List[DiffTestExecutor]

  def __init__(self, query_metric_filter: str, trace_filter: str,
               trace_processor_path: str, trace_descriptor: str,
               no_colors: bool):
    self.tests = read_all_tests(query_metric_filter, trace_filter, ROOT_DIR)
    self.trace_processor_path = trace_processor_path

    out_path = os.path.dirname(self.trace_processor_path)
    self.trace_descriptor_path = get_trace_descriptor_path(
        out_path, trace_descriptor)
    self.test_runners = []
    color_formatter = ColorFormatter(no_colors)
    for test in self.tests:
      self.test_runners.append(
          DiffTestExecutor(test, self.trace_processor_path,
                           self.trace_descriptor_path, color_formatter))

  def run_all_tests(self, metrics_descriptor: str, keep_input: bool,
                    rebase: bool) -> TestResults:
    perf_results = []
    failures = []
    rebased = []
    test_run_start = datetime.datetime.now()

    out_path = os.path.dirname(self.trace_processor_path)
    chrome_extensions = os.path.join(out_path, 'gen', 'protos', 'third_party',
                                     'chromium',
                                     'chrome_track_event.descriptor')
    test_extensions = os.path.join(out_path, 'gen', 'protos', 'perfetto',
                                   'trace', 'test_extensions.descriptor')

    with concurrent.futures.ProcessPoolExecutor() as e:
      fut = [
          e.submit(test.execute, [chrome_extensions, test_extensions],
                   metrics_descriptor, keep_input, rebase)
          for test in self.test_runners
      ]
      for res in concurrent.futures.as_completed(fut):
        test_name, res_str, result = res.result()
        sys.stderr.write(res_str)
        if not result or not result.passed:
          if rebase:
            rebased.append(test_name)
          failures.append(test_name)
        else:
          perf_results.append(result.perf_result)
    test_time_ms = int(
        (datetime.datetime.now() - test_run_start).total_seconds() * 1000)
    return TestResults(failures, perf_results, rebased, test_time_ms)
