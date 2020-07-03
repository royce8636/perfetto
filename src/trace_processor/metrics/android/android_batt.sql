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
CREATE VIEW battery_view AS
SELECT
  all_ts.ts as ts,
  current_avg_ua,
  capacity_percent,
  charge_uah,
  current_ua
FROM (
  SELECT distinct(ts) AS ts
  FROM counter c
  JOIN counter_track t on c.track_id = t.id
  WHERE name LIKE 'batt.%'
) AS all_ts
LEFT JOIN (
  SELECT ts, value AS current_avg_ua
  FROM counter c
  JOIN counter_track t on c.track_id = t.id
  WHERE name='batt.current.avg_ua'
) USING(ts)
LEFT JOIN (
  SELECT ts, value AS capacity_percent
  FROM counter c
  JOIN counter_track t on c.track_id = t.id
  WHERE name='batt.capacity_pct'
) USING(ts)
LEFT JOIN (
  SELECT ts, value AS charge_uah
  FROM counter c
  JOIN counter_track t on c.track_id = t.id
  WHERE name='batt.charge_uah'
) USING(ts)
LEFT JOIN (
  SELECT ts, value AS current_ua
  FROM counter c
  JOIN counter_track t on c.track_id = t.id
  WHERE name='batt.current_ua'
) USING(ts)
ORDER BY ts;

DROP TABLE IF EXISTS android_batt_wakelocks_raw_;
CREATE TABLE android_batt_wakelocks_raw_ AS
SELECT
  ts,
  dur,
  ts+dur AS ts_end
FROM slice
WHERE slice.name LIKE 'WakeLock %' AND dur != -1;

DROP TABLE IF EXISTS android_batt_wakelocks_labelled_;
CREATE TABLE android_batt_wakelocks_labelled_ AS
SELECT
  *,
  NOT EXISTS (
    SELECT *
    FROM android_batt_wakelocks_raw_ AS t2
    WHERE t2.ts < t1.ts
      AND t2.ts_end >= t1.ts
  ) AS no_overlap_at_start,
  NOT EXISTS (
    SELECT *
    FROM android_batt_wakelocks_raw_ AS t2
    WHERE t2.ts_end > t1.ts_end
      AND t2.ts <= t1.ts_end
  ) AS no_overlap_at_end
FROM android_batt_wakelocks_raw_ AS t1;

CREATE VIEW android_batt_wakelocks_merged AS
SELECT
  ts,
  (
    SELECT min(ts_end)
    FROM android_batt_wakelocks_labelled_ AS ends
    WHERE no_overlap_at_end
      AND ends.ts_end >= starts.ts
  ) AS ts_end
FROM android_batt_wakelocks_labelled_ AS starts
WHERE no_overlap_at_start;

SELECT RUN_METRIC('android/counter_span_view.sql',
  'table_name', 'screen_state',
  'counter_name', 'ScreenState');

CREATE VIEW android_batt_output AS
SELECT AndroidBatteryMetric(
  'battery_counters', (
    SELECT RepeatedField(
      AndroidBatteryMetric_BatteryCounters(
        'timestamp_ns', ts,
        'charge_counter_uah', charge_uah,
        'capacity_percent', capacity_percent,
        'current_ua', current_ua,
        'current_avg_ua', current_avg_ua
      )
    )
    FROM battery_view
  ),
  'battery_aggregates', (
    SELECT NULL_IF_EMPTY(AndroidBatteryMetric_BatteryAggregates(
      'total_screen_off_ns',
      SUM(CASE WHEN screen_state_val = 1.0 THEN dur ELSE 0 END),
      'total_screen_on_ns',
      SUM(CASE WHEN screen_state_val = 2.0 THEN dur ELSE 0 END),
      'total_screen_doze_ns',
      SUM(CASE WHEN screen_state_val = 3.0 THEN dur ELSE 0 END),
      'total_wakelock_ns', 
      (SELECT SUM(ts_end - ts) FROM android_batt_wakelocks_merged)
    ))
    FROM screen_state_span
  )
);
