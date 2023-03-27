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
"""Contains tables for relevant for TODO."""

from python.generators.trace_processor_table.public import Column as C
from python.generators.trace_processor_table.public import CppDouble
from python.generators.trace_processor_table.public import ColumnFlag
from python.generators.trace_processor_table.public import CppInt64
from python.generators.trace_processor_table.public import CppOptional
from python.generators.trace_processor_table.public import Table
from python.generators.trace_processor_table.public import TableDoc
from python.generators.trace_processor_table.public import CppTableId
from python.generators.trace_processor_table.public import CppUint32

from src.trace_processor.tables.track_tables import COUNTER_TRACK_TABLE

COUNTER_TABLE = Table(
    class_name='CounterTable',
    sql_name='counter',
    columns=[
        C('ts', CppInt64(), flags=ColumnFlag.SORTED),
        C('track_id', CppTableId(COUNTER_TRACK_TABLE)),
        C('value', CppDouble()),
        C('arg_set_id', CppOptional(CppUint32())),
    ],
    tabledoc=TableDoc(
        doc='''''',
        group='Events',
        columns={
            'ts': '''''',
            'track_id': '''''',
            'value': '''''',
            'arg_set_id': '''''',
        }))

# Keep this list sorted.
ALL_TABLES = [
    COUNTER_TABLE,
]
