--
-- Copyright 2019 The Android Open Source Project
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
  ts BIGINT,
  dur BIGINT,
  part BIGINT,
  PRIMARY KEY (part, ts)
) without rowid;

create table t2(
  ts BIGINT,
  dur BIGINT,
  part BIGINT,
  PRIMARY KEY (part, ts)
) without rowid;

-- Insert a single row into t1.
INSERT INTO t1(ts, dur, part)
VALUES (500, 100, 10);

-- t2 is empty.

create virtual table sp using span_outer_join(t1 PARTITIONED part,
                                             t2 PARTITIONED part);

select * from sp;
