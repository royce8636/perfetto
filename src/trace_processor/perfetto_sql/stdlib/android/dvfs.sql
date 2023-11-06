--
-- Copyright 2023 The Android Open Source Project
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

-- Dvfs counter with duration.
CREATE PERFETTO VIEW android_dvfs_counters(
  -- Counter name.
  name STRING,
  -- Timestamp when counter value changed.
  ts INT,
  -- Counter value.
  value INT,
  -- Counter duration.
  dur INT
) AS
SELECT
  counter_track.name,
  counter.ts,
  counter.value,
  LEAD(counter.ts, 1, (SELECT end_ts + 1 FROM trace_bounds))
  OVER (PARTITION BY counter_track.id ORDER BY counter.ts) - counter.ts AS dur
FROM counter
JOIN counter_track
  ON counter.track_id = counter_track.id
WHERE
  counter_track.name IN (
    'domain@0 Frequency',
    'domain@1 Frequency',
    'domain@2 Frequency',
    '17000010.devfreq_mif Frequency',
    '17000020.devfreq_int Frequency',
    '17000090.devfreq_dsu Frequency',
    '170000a0.devfreq_bci Frequency',
    'dsu_throughput Frequency',
    'bus_throughput Frequency',
    'cpu0dsu Frequency',
    'cpu1dsu Frequency',
    'cpu2dsu Frequency',
    'cpu3dsu Frequency',
    'cpu4dsu Frequency',
    'cpu5dsu Frequency',
    'cpu6dsu Frequency',
    'cpu7dsu Frequency',
    'cpu8dsu Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu0_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu1_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu2_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu3_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu4_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu5_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu6_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu7_memlat@17000010 Frequency',
    'gs_memlat_devfreq:devfreq_mif_cpu8_memlat@17000010 Frequency')
ORDER BY ts;

-- Aggregates dvfs counter slice for statistic.
CREATE PERFETTO TABLE android_dvfs_counter_stats(
  -- Counter name on which all the other values are aggregated on.
  name STRING,
  -- Max of all counter values for the counter name.
  max INT,
  -- Min of all counter values for the counter name.
  min INT,
  -- Duration between the first and last counter value for the counter name.
  dur INT,
  -- Weighted avergate of all the counter values for the counter name.
  wgt_avg FLOAT
) AS
SELECT
  name,
  MAX(value) AS max,
  MIN(value) AS min,
  (MAX(ts) - MIN(ts)) AS dur,
  (SUM(dur * value) / SUM(dur)) AS wgt_avg
FROM android_dvfs_counters
WHERE android_dvfs_counters.dur > 0
GROUP BY name;


-- Aggregates dvfs counter slice for residency
CREATE PERFETTO VIEW android_dvfs_counter_residency(
  -- Id of the corresponding slice in slices table.
  slice_id INT,
  -- CPU that entered hypervisor.
  cpu INT,
  -- Timestamp when CPU entered hypervisor (in nanoseconds).
  ts INT,
  -- How much time CPU spent in hypervisor (in nanoseconds).
  dur INT,
  -- Reason for entering hypervisor (e.g. host_hcall, host_mem_abort), or NULL if unknown.
  reason STRING
) AS
WITH
total AS (
  SELECT
    name,
    SUM(dur) AS dur
  FROM android_dvfs_counters
  WHERE dur > 0
  GROUP BY name
)
SELECT
  android_dvfs_counters.name,
  android_dvfs_counters.value,
  SUM(android_dvfs_counters.dur) AS dur,
  (SUM(android_dvfs_counters.dur) * 100.0 / total.dur) AS pct
FROM android_dvfs_counters
JOIN total
  USING (name)
WHERE android_dvfs_counters.dur > 0
GROUP BY 1, 2;
