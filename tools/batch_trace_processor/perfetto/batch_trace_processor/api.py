#!/usr/bin/env python3
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


"""Contains classes for BatchTraceProcessor API."""

import concurrent.futures as cf
import dataclasses as dc
from typing import Any, Callable, Dict, Tuple, Union, List

import pandas as pd

from perfetto.trace_processor import TraceProcessor
from perfetto.trace_processor import TraceProcessorException


@dc.dataclass
class _TpArg:
  bin_path: str
  verbose: bool
  file: str


@dc.dataclass
class TraceFile:
  trace_path: str
  args: Dict[str, str]


def _create_trace_file(path_or_trace_file: Union[str, TraceFile]) -> TraceFile:
  if isinstance(path_or_trace_file, str):
    return TraceFile(trace_path=path_or_trace_file, args={})
  return path_or_trace_file


class BatchTraceProcessor:
  """Run ad-hoc SQL queries across many Perfetto traces.

  Usage:
    with BatchTraceProcessor(files=files) as btp:
      dfs = btp.query('select * from slice')
      for df in dfs:
        print(df)
  """

  def __init__(self,
               files: Union[List[TraceFile], List[str]],
               bin_path: str = None,
               verbose: bool = False):
    """Creates a batch trace processor instance.

    BatchTraceProcessor is the blessed way of running ad-hoc queries in
    Python across many traces.

    Args:
      files: Either a list of trace file paths or a list of TraceFile objects
        indicating the traces to load into this batch trace processor instance.
      bin_path: Optional path to a trace processor shell binary to use to
        load the traces.
      verbose: Optional flag indiciating whether verbose trace processor
        output should be printed to stderr.
    """
    self.tps = None
    self.closed = False
    self.executor = cf.ThreadPoolExecutor()

    self.files = [_create_trace_file(file) for file in files]

    def create_tp(arg: _TpArg) -> TraceProcessor:
      return TraceProcessor(
          file_path=arg.file, bin_path=arg.bin_path, verbose=arg.verbose)

    tp_args = [
        _TpArg(bin_path, verbose, file.trace_path) for file in self.files
    ]
    self.tps = list(self.executor.map(create_tp, tp_args))

  def metric(self, metrics: List[str]):
    """Computes the provided metrics.

    The computation happens in parallel across all the traces.

    Args:
      metrics: A list of valid metrics as defined in TraceMetrics

    Returns:
      A list of TraceMetric protos (one for each trace).
    """
    return self.execute(lambda tp: tp.metric(metrics))

  def query(self, sql: str):
    """Executes the provided SQL statement (returning a single row).

    The execution happens in parallel across all the traces.

    Args:
      sql: The SQL statement to execute.

    Returns:
      A list of Pandas dataframes with the result of executing the query (one
      per trace).

    Raises:
      TraceProcessorException: An error occurred running the query.
    """
    return self.execute(lambda tp: tp.query(sql).as_pandas_dataframe())

  def query_and_flatten(self, sql: str):
    """Executes the provided SQL statement and flattens the result.

    The execution happens in parallel across all the traces and the
    resulting Pandas dataframes are flattened into a single dataframe.

    Args:
      sql: The SQL statement to execute.

    Returns:
      A Pandas dataframe containing the result of executing the query across all
      the traces. The dataframe will have an additional column 'trace_path'
      indicating the trace file associated with that row. Also, if |TraceFile|
      objects were passed to the constructor, the contents of the |args|
      dictionary will also be emitted as other columns (key being column name,
      value being the value in the dataframe).

      For example:
        files = [TraceFile(trace_path='/tmp/path', args={"foo": "bar"})]
        with BatchTraceProcessor(files=files) as btp:
          df = btp.query_and_flatten('select count(1) as cnt from slice')

      Then df will look like this:
        cnt           trace_path              foo
        100           /tmp/path               bar

    Raises:
      TraceProcessorException: An error occurred running the query.
    """
    return self.execute_and_flatten(lambda tp: tp.query(sql).
                                    as_pandas_dataframe())

  def query_single_result(self, sql: str):
    """Executes the provided SQL statement (returning a single row).

    The execution happens in parallel across all the traces.

    Args:
      sql: The SQL statement to execute. This statement should return exactly
        one row on any trace.

    Returns:
      A list of values with the result of executing the query (one per trace).

    Raises:
      TraceProcessorException: An error occurred running the query or more than
        one result was returned.
    """

    def query_single_result_inner(tp):
      df = tp.query(sql).as_pandas_dataframe()
      if len(df.index) != 1:
        raise TraceProcessorException("Query should only return a single row")

      if len(df.columns) != 1:
        raise TraceProcessorException(
            "Query should only return a single column")

      return df.iloc[0, 0]

    return self.execute(query_single_result_inner)

  def execute(self, fn: Callable[[TraceProcessor], Any]) -> List[Any]:
    """Executes the provided function.

    The execution happens in parallel across all the trace processor instances
    owned by this object.

    Args:
      fn: The function to execute.

    Returns:
      A list of values with the result of executing the fucntion (one per
      trace).
    """
    return list(self.executor.map(fn, self.tps))

  def execute_and_flatten(self, fn: Callable[[TraceProcessor], pd.DataFrame]
                         ) -> pd.DataFrame:
    """Executes the provided function and flattens the result.

    The execution happens in parallel across all the trace processor
    instances owned by this object and the returned Pandas dataframes are
    flattened into a single dataframe.

    Args:
      fn: The function to execute which returns a Pandas dataframe.

    Returns:
      A Pandas dataframe containing the result of executing the query across all
      the traces. Extra columns containing the file path and args will
      be added to the dataframe (see |query_and_flatten| for details).
    """

    def wrapped(pair: Tuple[TraceProcessor, TraceFile]):
      (tp, file) = pair
      df = fn(tp)
      df["trace_path"] = file.trace_path
      for key, value in file.args.items():
        df[key] = value
      return df

    df = pd.concat(list(self.executor.map(wrapped, zip(self.tps, self.files))))
    return df.reset_index(drop=True)

  def close(self):
    """Closes this batch trace processor instance.

    This closes all spawned trace processor instances, releasing all the memory
    and resources those instances take.

    No further calls to other methods in this class should be made after
    calling this method.
    """
    if self.closed:
      return
    self.closed = True
    self.executor.shutdown()

    if self.tps:
      for tp in self.tps:
        tp.close()

  def __enter__(self):
    return self

  def __exit__(self, a, b, c):
    del a, b, c  # Unused.
    self.close()
    return False

  def __del__(self):
    self.close()
