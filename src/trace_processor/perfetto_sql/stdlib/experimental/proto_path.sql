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

-- Creates a Stack consisting of one frame for a path in the
-- EXPERIMENTAL_PROTO_PATH table.
--
-- @ret BYTES    Stack with one frame
CREATE PERFETTO FUNCTON EXPERIMENTAL_PROTO_PATH_TO_FRAME(
-- Id of the path in EXPERIMENTAL_PROTO_PATH.
  path_id LONG)
RETURNS BYTES AS
SELECT
  CAT_STACKS(
    'event.name:' || EXTRACT_ARG(arg_set_id, 'event.name'),
    'event.category:' || EXTRACT_ARG(arg_set_id, 'event.category'),
    field_name,
    field_type)
FROM EXPERIMENTAL_PROTO_PATH
WHERE id = $path_id;

-- Creates a Stack following the parent relations in EXPERIMENTAL_PROTO_PATH
-- table starting at the given path_id.
--
-- @ret BYTES    Stack
CREATE PERFETTO FUNCTION experimental_proto_path_to_stack(
-- Id of the path in EXPERIMENTAL_PROTO_PATH that will be the leaf in the returned stack.
  path_id LONG)
RETURNS BYTES AS
WITH
  R AS (
    -- Starting at the given path_id generate a stack
    SELECT
      EXPERIMENTAL_PROTO_PATH_TO_FRAME($path_id) AS stack,
      parent_id AS parent_id
    FROM EXPERIMENTAL_PROTO_PATH AS p
    WHERE id = $path_id
    UNION ALL
    -- And recursively add parent paths to the stack
    SELECT
      CAT_STACKS(
        EXPERIMENTAL_PROTO_PATH_TO_FRAME(p.id),
        c.stack)
        AS stack,
      p.parent_id AS parent_id
    FROM EXPERIMENTAL_PROTO_PATH AS p, R AS c
    WHERE p.id = c.parent_id
  )
-- Select only the last row in the recursion (the one that stopped it because
-- it had no parent, i.e. the root) as this will be the row that has the full
-- stack. All the others will only have partial stacks.
SELECT stack
FROM R
WHERE
  parent_id IS NULL;