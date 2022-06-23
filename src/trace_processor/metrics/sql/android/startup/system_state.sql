--
-- Copyright 2022 The Android Open Source Project
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

-- Functions useful for filling the SystemState proto which gives
-- context to what was happening on the system during a startup.

-- Given a launch id and process name glob, returns whether a process with
-- that name was running on a CPU concurrent to that launch.
SELECT CREATE_FUNCTION(
  'IS_PROCESS_RUNNING_CONCURRENT_TO_LAUNCH(launch_id INT, process_glob STRING)',
  'BOOL',
  '
    SELECT EXISTS(
      SELECT sched.dur
      FROM sched
      JOIN thread USING (utid)
      JOIN process USING (upid)
      JOIN (
        SELECT ts, ts_end
        FROM launches
        WHERE id = $launch_id
      ) launch
      WHERE
        process.name GLOB $process_glob AND
        sched.ts BETWEEN launch.ts AND launch.ts_end
      LIMIT 1
    )
  '
);
