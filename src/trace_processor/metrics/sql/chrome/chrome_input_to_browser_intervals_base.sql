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
-- WARNING: This metric should not be used as a source of truth. It is under
--          active development and the values & meaning might change without
--          notice.

-- Given a slice id of an event, get timestamp of the most recent flow
-- event on the Chrome IO thread that preceded this slice.
-- This helps us identify the last flow event on the IO thread before
-- letting the browser main thread be aware of input.
-- We need this for flings (generated by the GPU process) and blocked
-- touch moves that are forwarded from the renderer.
-- Returning the slice id for the flow_out on the chrome IO thread.
CREATE PERFETTO FUNCTION {{function_prefix}}PRECEDING_IO_THREAD_EVENT_FLOW_ID(id LONG)
RETURNS LONG AS
SELECT MAX(flow.slice_out) AS id
FROM PRECEDING_FLOW(($id)) flow;

-- Returns a Chrome task which contains the given slice.
CREATE PERFETTO FUNCTION {{function_prefix}}GET_ENCLOSING_CHROME_TASK_NAME(
  slice_id LONG
)
RETURNS STRING AS
SELECT
  task.name
FROM ancestor_slice($slice_id)
JOIN chrome_tasks task USING (id)
LIMIT 1;

CREATE PERFETTO FUNCTION {{function_prefix}}GET_SCROLL_TYPE(
  blocked_gesture BOOL,
  task_name STRING
)
RETURNS STRING AS
SELECT
  CASE WHEN ($blocked_gesture)
  THEN (SELECT
          CASE WHEN ($task_name) glob "viz.mojom.BeginFrameObserver *"
          THEN "fling"
          WHEN ($task_name) glob "blink.mojom.WidgetInputHandler *"
          THEN "blocking_touch_move"
          ELSE "unknown" END)
  ELSE "regular" END AS delay_type;

-- Get all InputLatency::GestureScrollUpdate events, to use their
-- flows later on to decide how much time we waited from queueing the event
-- until we started processing it.
DROP VIEW IF EXISTS chrome_valid_gesture_updates;
CREATE VIEW chrome_valid_gesture_updates
AS
SELECT
  name,
  EXTRACT_ARG(
    arg_set_id, 'chrome_latency_info.trace_id') AS trace_id,
  ts,
  id,
  dur
FROM
  {{slice_table_name}}
WHERE
  name = 'InputLatency::GestureScrollUpdate'
  AND EXTRACT_ARG(
    arg_set_id, "chrome_latency_info.is_coalesced")
  = 0
ORDER BY trace_id;

-- Get all chrome_latency_info_for_gesture_slices where trace_ids are not -1,
-- as those are faulty, then join with the GestureScrollUpdate table to get
-- only slices associated with update events.
DROP VIEW IF EXISTS chrome_flow_slices_for_gestures;
CREATE VIEW chrome_flow_slices_for_gestures
AS
SELECT
  s.ts,
  EXTRACT_ARG(
    s.arg_set_id, 'chrome_latency_info.trace_id') AS trace_id,
  s.arg_set_id,
  s.id,
  s.track_id,
  s.dur
FROM
  {{slice_table_name}} s
JOIN chrome_valid_gesture_updates
  ON
    chrome_valid_gesture_updates.trace_id = EXTRACT_ARG(
      s.arg_set_id, 'chrome_latency_info.trace_id')
    AND s.ts >= chrome_valid_gesture_updates.ts
    AND s.ts + s.dur
    <= chrome_valid_gesture_updates.ts + chrome_valid_gesture_updates.dur
WHERE
  s.name = 'LatencyInfo.Flow'
  AND trace_id != -1;

-- Tie chrome_latency_info_for_gesture_slices slices to processes to avoid
-- calculating intervals per process as multiple chrome instances can be up
-- on system traces.
DROP VIEW IF EXISTS chrome_flow_slices_for_gestures_tied_process;
CREATE VIEW chrome_flow_slices_for_gestures_tied_process
AS
SELECT
  ts,
  trace_id,
  arg_set_id,
  chrome_flow_slices_for_gestures.id,
  dur,
  thread.upid
FROM
  chrome_flow_slices_for_gestures
JOIN thread_track ON chrome_flow_slices_for_gestures.track_id = thread_track.id
JOIN thread ON thread_track.utid = thread.utid
  AND is_main_thread;

-- Index all flows per trace_id, to get the first flow event per input
-- GestureScrollUpdate, this will later be used to calculate the time
-- from receiving input to the first flow event appearing.
DROP TABLE IF EXISTS chrome_indexed_flow_per_gesture;
CREATE PERFETTO TABLE chrome_indexed_flow_per_gesture
AS
SELECT
  ts,
  trace_id,
  arg_set_id,
  id,
  ROW_NUMBER()
  OVER (
    PARTITION BY trace_id, upid
    ORDER BY
      ts ASC
  ) AS flow_order,
  upid,
  dur
FROM
  chrome_flow_slices_for_gestures_tied_process;

-- TODO(b/235067134) all the previous views including this one are
-- reimplementations of gesture_jank.sql with less restrictions, let's
-- merge both of them into one script or make this script a base for the
-- other.
-- Get the first flow event per gesture.
DROP VIEW IF EXISTS chrome_first_flow_per_gesture;
CREATE VIEW chrome_first_flow_per_gesture
AS
SELECT
  *
FROM
  chrome_indexed_flow_per_gesture
WHERE
  flow_order = 1;

-- The decision for processing on the browser main thread for a frame can be
-- instant, or delayed by the renderer in cases where the renderer needs to
-- decide whether the touch move is an ScrollUpdate or not, and in other cases
-- for flings, the scroll itself will be generated by the viz compositor thread
-- on each vsync interval.
DROP VIEW IF EXISTS chrome_categorized_first_flow_events;
CREATE VIEW chrome_categorized_first_flow_events
AS
SELECT
  *,
  NOT COALESCE((
    SELECT
      COUNT()
    FROM
      ancestor_slice(chrome_first_flow_per_gesture.id) ancestor_slices
    WHERE
      -- sendTouchEvent means the event wasn't delayed by the renderer
      -- and is not a fling generated by the viz compositor thread(GPU process).
      ancestor_slices.name = "sendTouchEvent"
  )
  = 1, FALSE) AS blocked_gesture
FROM
  chrome_first_flow_per_gesture;

-- For cases where it's not blocked, get the timestamp of input as the
-- beginning of time we theoretically could have started processing
-- the input event, which is the timestamp of the GestureScrollUpdate event
-- otherwise fall back to the top level slice to check the timestamp
-- of it's calling flow
DROP VIEW IF EXISTS chrome_input_to_browser_interval_slice_ids;
CREATE VIEW chrome_input_to_browser_interval_slice_ids
AS
SELECT
  chrome_categorized_first_flow_events.id AS window_end_id,
  chrome_categorized_first_flow_events.ts AS window_end_ts,
  -- If blocked, get the flow_out slice's timestamp as our beginning
  -- given that the flow_in is coming to our toplevel parent task
  CASE
    WHEN blocked_gesture
      THEN
      {{function_prefix}}PRECEDING_IO_THREAD_EVENT_FLOW_ID(chrome_categorized_first_flow_events.id)
    ELSE
      -- TODO(b/236590359): is selecting here better or join and ordering by?
      -- Let's benchmark and modify accordingly.
      (
        SELECT
          chrome_gestures.id
        FROM
          chrome_valid_gesture_updates chrome_gestures
        WHERE
          chrome_gestures.trace_id = chrome_categorized_first_flow_events.trace_id
          AND chrome_gestures.ts <= chrome_categorized_first_flow_events.ts
          AND chrome_gestures.ts + chrome_gestures.dur >= chrome_categorized_first_flow_events.ts
          + chrome_categorized_first_flow_events.dur
      )
  END AS window_start_id,
  blocked_gesture,
  upid
FROM
  chrome_categorized_first_flow_events;