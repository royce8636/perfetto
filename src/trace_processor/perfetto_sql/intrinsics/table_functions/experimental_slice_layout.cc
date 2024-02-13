/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/trace_processor/perfetto_sql/intrinsics/table_functions/experimental_slice_layout.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "perfetto/base/logging.h"
#include "perfetto/base/status.h"
#include "perfetto/ext/base/status_or.h"
#include "perfetto/ext/base/string_splitter.h"
#include "perfetto/ext/base/string_utils.h"
#include "perfetto/ext/base/string_view.h"
#include "perfetto/trace_processor/basic_types.h"
#include "src/trace_processor/containers/string_pool.h"
#include "src/trace_processor/db/column_storage.h"
#include "src/trace_processor/db/table.h"
#include "src/trace_processor/perfetto_sql/intrinsics/table_functions/tables_py.h"
#include "src/trace_processor/storage/trace_storage.h"
#include "src/trace_processor/tables/slice_tables_py.h"

namespace perfetto::trace_processor {
namespace tables {

ExperimentalSliceLayoutTable::~ExperimentalSliceLayoutTable() = default;
}

namespace {

struct GroupInfo {
  GroupInfo(int64_t _start, int64_t _end, uint32_t _max_depth)
      : start(_start), end(_end), max_depth(_max_depth) {}
  int64_t start;
  int64_t end;
  uint32_t layout_depth = 0;
  uint32_t max_depth;
};

}  // namespace

ExperimentalSliceLayout::ExperimentalSliceLayout(
    StringPool* string_pool,
    const tables::SliceTable* table)
    : string_pool_(string_pool),
      slice_table_(table),
      empty_string_id_(string_pool_->InternString("")) {}
ExperimentalSliceLayout::~ExperimentalSliceLayout() = default;

Table::Schema ExperimentalSliceLayout::CreateSchema() {
  return tables::ExperimentalSliceLayoutTable::ComputeStaticSchema();
}

std::string ExperimentalSliceLayout::TableName() {
  return tables::ExperimentalSliceLayoutTable::Name();
}

uint32_t ExperimentalSliceLayout::EstimateRowCount() {
  return slice_table_->row_count();
}

base::StatusOr<std::unique_ptr<Table>> ExperimentalSliceLayout::ComputeTable(
    const std::vector<SqlValue>& arguments) {
  PERFETTO_CHECK(arguments.size() == 1);

  if (arguments[0].type != SqlValue::Type::kString) {
    return base::ErrStatus("invalid input track id list");
  }

  std::string filter_string = arguments[0].AsString();
  std::set<TrackId> selected_tracks;
  for (base::StringSplitter sp(filter_string, ','); sp.Next();) {
    std::optional<uint32_t> maybe = base::CStringToUInt32(sp.cur_token());
    if (maybe) {
      selected_tracks.insert(TrackId{maybe.value()});
    }
  }

  StringPool::Id filter_id =
      string_pool_->InternString(base::StringView(filter_string));

  // Try and find the table in the cache.
  auto cache_it = layout_table_cache_.find(filter_id);
  if (cache_it != layout_table_cache_.end()) {
    return std::make_unique<Table>(cache_it->second->Copy());
  }

  // Find all the slices for the tracks we want to filter and create a vector of
  // row numbers out of them.
  std::vector<tables::SliceTable::RowNumber> rows;
  for (auto it = slice_table_->IterateRows(); it; ++it) {
    if (selected_tracks.count(it.track_id())) {
      rows.emplace_back(it.row_number());
    }
  }

  // Compute the table and add it to the cache for future use.
  std::unique_ptr<Table> layout_table =
      ComputeLayoutTable(std::move(rows), filter_id);
  auto res = layout_table_cache_.emplace(filter_id, std::move(layout_table));

  return std::make_unique<Table>(res.first->second->Copy());
}

// Build up a table of slice id -> root slice id by observing each
// (id, opt_parent_id) pair in order.
tables::SliceTable::Id ExperimentalSliceLayout::InsertSlice(
    std::map<tables::SliceTable::Id, tables::SliceTable::Id>& id_map,
    tables::SliceTable::Id id,
    std::optional<tables::SliceTable::Id> parent_id) {
  if (parent_id) {
    tables::SliceTable::Id root_id = id_map[parent_id.value()];
    id_map[id] = root_id;
    return root_id;
  }
  id_map[id] = id;
  return id;
}

// The problem we're trying to solve is this: given a number of tracks each of
// which contain a number of 'stalactites' - depth 0 slices and all their
// children - layout the stalactites to minimize vertical depth without
// changing the horizontal (time) position. So given two tracks:
// Track A:
//     aaaaaaaaa       aaa
//                      aa
//                       a
// Track B:
//      bbb       bbb    bbb
//       b         b      b
// The result could be something like:
//     aaaaaaaaa  bbb  aaa
//                 b    aa
//      bbb              a
//       b
//                       bbb
//                        b
// We do this by computing an additional column: layout_depth. layout_depth
// tells us the vertical position of each slice in each stalactite.
//
// The algorithm works in three passes:
// 1. For each stalactite find the 'bounding box' (start, end, & max depth)
// 2. Considering each stalactite bounding box in start ts order pick a
//    layout_depth for the root slice of stalactite to avoid collisions with
//    all previous stalactite's we've considered.
// 3. Go though each slice and give it a layout_depth by summing it's
//    current depth and the root layout_depth of the stalactite it belongs to.
//
std::unique_ptr<Table> ExperimentalSliceLayout::ComputeLayoutTable(
    std::vector<tables::SliceTable::RowNumber> rows,
    StringPool::Id filter_id) {
  std::map<tables::SliceTable::Id, GroupInfo> groups;
  // Map of id -> root_id
  std::map<tables::SliceTable::Id, tables::SliceTable::Id> id_map;

  // Step 1:
  // Find the bounding box (start ts, end ts, and max depth) for each group
  // TODO(lalitm): Update this to use iterator (as this code will be slow after
  // the event table is implemented)
  for (tables::SliceTable::RowNumber i : rows) {
    auto ref = i.ToRowReference(*slice_table_);

    tables::SliceTable::Id id = ref.id();
    uint32_t depth = ref.depth();
    int64_t start = ref.ts();
    int64_t dur = ref.dur();
    int64_t end = dur == -1 ? std::numeric_limits<int64_t>::max() : start + dur;
    InsertSlice(id_map, id, ref.parent_id());
    std::map<tables::SliceTable::Id, GroupInfo>::iterator it;
    bool inserted;
    std::tie(it, inserted) = groups.emplace(
        std::piecewise_construct, std::forward_as_tuple(id_map[id]),
        std::forward_as_tuple(start, end, depth));
    if (!inserted) {
      it->second.max_depth = std::max(it->second.max_depth, depth);
      it->second.end = std::max(it->second.end, end);
    }
  }

  // Sort the groups by ts
  std::vector<GroupInfo*> sorted_groups;
  sorted_groups.resize(groups.size());
  size_t idx = 0;
  for (auto& group : groups) {
    sorted_groups[idx++] = &group.second;
  }
  std::sort(std::begin(sorted_groups), std::end(sorted_groups),
            [](const GroupInfo* group1, const GroupInfo* group2) {
              return group1->start < group2->start;
            });

  // Step 2:
  // Go though each group and choose a depth for the root slice.
  // We keep track of those groups where the start time has passed but the
  // end time has not in this vector:
  std::vector<GroupInfo*> still_open;
  for (GroupInfo* group : sorted_groups) {
    int64_t start = group->start;
    uint32_t max_depth = group->max_depth;

    // Discard all 'closed' groups where that groups end_ts is < our start_ts:
    {
      auto it = still_open.begin();
      while (it != still_open.end()) {
        if ((*it)->end <= start) {
          it = still_open.erase(it);
        } else {
          ++it;
        }
      }
    }

    uint32_t layout_depth = 0;

    // In a pathological case you can end up stacking up slices forever
    // triggering n^2 behaviour below. In those cases we want to give
    // up on trying to find a pretty (height minimizing ) layout and
    // just find *some* layout. To do that we start looking for
    // a layout depth below the maximum open group which should
    // immediately succeed.
    if (still_open.size() > 500) {
      for (const auto& open : still_open) {
        layout_depth =
            std::max(layout_depth, open->layout_depth + open->max_depth);
      }
    }

    // Find a start layout depth for this group s.t. our start depth +
    // our max depth will not intersect with the start depth + max depth for
    // any of the open groups:
    bool done = false;
    while (!done) {
      done = true;
      uint32_t start_depth = layout_depth;
      uint32_t end_depth = layout_depth + max_depth;
      for (const auto& open : still_open) {
        uint32_t open_start_depth = open->layout_depth;
        uint32_t open_end_depth = open->layout_depth + open->max_depth;
        bool fully_above_open = end_depth < open_start_depth;
        bool fully_below_open = open_end_depth < start_depth;
        if (!fully_above_open && !fully_below_open) {
          // This is extremely dumb, we can make a much better guess for what
          // depth to try next but it is a little complicated to get right.
          layout_depth++;
          done = false;
          break;
        }
      }
    }

    // Add this group to the open groups & re
    still_open.push_back(group);

    // Set our root layout depth:
    group->layout_depth = layout_depth;
  }

  // Step 3: Add the two new columns layout_depth and filter_track_ids:
  ColumnStorage<uint32_t> layout_depth_column;
  ColumnStorage<StringPool::Id> filter_column;

  for (tables::SliceTable::RowNumber i : rows) {
    auto ref = i.ToRowReference(*slice_table_);

    // Each slice depth is it's current slice depth + root slice depth of the
    // group:
    uint32_t group_depth = groups.at(id_map[ref.id()]).layout_depth;
    layout_depth_column.Append(ref.depth() + group_depth);
    // We must set this to the value we got in the constraint to ensure our
    // rows are not filtered out:
    filter_column.Append(filter_id);
  }

  return tables::ExperimentalSliceLayoutTable::SelectAndExtendParent(
      *slice_table_, std::move(rows), std::move(layout_depth_column),
      std::move(filter_column));
}

}  // namespace perfetto::trace_processor
