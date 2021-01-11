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

SELECT RUN_METRIC('android/process_metadata.sql');
SELECT RUN_METRIC('android/process_mem.sql');

DROP VIEW IF EXISTS java_heap_stats_output;
CREATE VIEW java_heap_stats_output AS
WITH
-- Base view
base_stat_counts AS (
  SELECT
    upid,
    graph_sample_ts,
    SUM(self_size) AS total_size,
    COUNT(1) AS total_obj_count,
    SUM(CASE reachable WHEN TRUE THEN self_size ELSE 0 END) AS reachable_size,
    SUM(CASE reachable WHEN TRUE THEN 1 ELSE 0 END) AS reachable_obj_count
  FROM heap_graph_object
  GROUP BY 1, 2
),
heap_roots AS (
  SELECT
    upid,
    graph_sample_ts,
    root_type,
    IFNULL(t.deobfuscated_name, t.name) type_name,
    COUNT(1) obj_count
  FROM heap_graph_object o
  JOIN heap_graph_class t ON o.type_id = t.id
  -- Classes are going to be particularly spammy and uninteresting
  -- from a memory analysis perspective (compared e.g. to local jni roots)
  WHERE root_type IS NOT NULL AND root_type != 'ROOT_STICKY_CLASS'
  GROUP BY 1, 2, 3, 4
  ORDER BY obj_count DESC
),
heap_roots_proto AS (
  SELECT
    upid,
    graph_sample_ts,
    RepeatedField(JavaHeapStats_HeapRoots(
      'root_type', root_type,
      'type_name', type_name,
      'obj_count', obj_count
    )) roots
  FROM heap_roots
  GROUP BY 1, 2
),
base_stats AS (
  SELECT * FROM base_stat_counts JOIN heap_roots_proto USING (upid, graph_sample_ts)
),
-- Find closest value
closest_anon_swap AS (
  SELECT
    upid,
    graph_sample_ts,
    (
      SELECT anon_swap_val
      FROM (
        SELECT
          ts, dur,
          CAST(anon_and_swap_val AS INTEGER) anon_swap_val,
          ABS(ts - base_stats.graph_sample_ts) diff
        FROM anon_and_swap_span
        WHERE upid = base_stats.upid)
      WHERE
        (graph_sample_ts >= ts AND graph_sample_ts < ts + dur)
         -- If the first memory sample for the UPID comes *after* the heap profile
         -- accept it if close (500ms)
        OR (graph_sample_ts < ts AND diff <= 500 * 1e6)
      ORDER BY diff LIMIT 1
    ) val
  FROM base_stats
),
-- Group by upid
heap_graph_sample_protos AS (
  SELECT
    base_stats.upid,
    RepeatedField(JavaHeapStats_Sample(
      'ts', graph_sample_ts,
      'heap_size', total_size,
      'obj_count', total_obj_count,
      'reachable_heap_size', reachable_size,
      'reachable_obj_count', reachable_obj_count,
      'roots', roots,
      'anon_rss_and_swap_size', closest_anon_swap.val
    )) sample_protos
  FROM base_stats
  LEFT JOIN closest_anon_swap USING (upid, graph_sample_ts)
  GROUP BY 1
)
SELECT JavaHeapStats(
  'instance_stats', RepeatedField(JavaHeapStats_InstanceStats(
    'upid', upid,
    'process', process_metadata.metadata,
    'samples', sample_protos
  )))
FROM heap_graph_sample_protos JOIN process_metadata USING (upid);
