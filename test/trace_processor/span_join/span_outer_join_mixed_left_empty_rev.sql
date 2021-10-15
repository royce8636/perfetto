--
-- Copyright 2021 The Android Open Source Project
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
create table t1(
  ts BIG INT,
  dur BIG INT,
  part BIG INT,
  PRIMARY KEY (part, ts)
) without rowid;

create table t2(
  ts BIG INT,
  dur BIG INT,
  PRIMARY KEY (ts)
) without rowid;

INSERT INTO t1(ts, dur, part)
VALUES
(100, 400, 0),
(100, 50, 1),
(600, 100, 1);

create virtual table sp using span_outer_join(t2, t1 PARTITIONED part);

select * from sp;
