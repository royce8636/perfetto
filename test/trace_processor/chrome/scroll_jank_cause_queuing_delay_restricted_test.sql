--
-- Copyright 2020 The Android Open Source Project
--
-- Licensed under the Apache License, Version 2.0 (the 'License');
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an 'AS IS' BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
SELECT RUN_METRIC('chrome/scroll_jank_cause_queuing_delay.sql');

SELECT
  process_name,
  thread_name,
  trace_id,
  jank,
  dur_overlapping_ns,
  restricted_metric_name
FROM scroll_jank_cause_queuing_delay
WHERE trace_id = 2918 OR trace_id = 2926
ORDER BY trace_id ASC, ts ASC;
