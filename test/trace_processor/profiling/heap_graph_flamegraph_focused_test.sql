SELECT
  id,
  depth,
  name,
  count,
  cumulative_count,
  size,
  cumulative_size,
  parent_id
FROM experimental_flamegraph
WHERE upid = (SELECT max(upid) FROM heap_graph_object)
  AND profile_type = 'graph'
  AND ts = (SELECT max(graph_sample_ts) FROM heap_graph_object)
  AND focus_str = 'left'
LIMIT 10;
