/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "src/trace_processor/db/table.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "perfetto/base/logging.h"
#include "src/trace_processor/containers/row_map.h"
#include "src/trace_processor/containers/string_pool.h"
#include "src/trace_processor/db/column.h"
#include "src/trace_processor/db/column/data_layer.h"
#include "src/trace_processor/db/column/types.h"
#include "src/trace_processor/db/column_storage_overlay.h"

namespace perfetto::trace_processor {

bool Table::kUseFilterV2 = true;

Table::Table(StringPool* pool,
             uint32_t row_count,
             std::vector<ColumnLegacy> columns,
             std::vector<ColumnStorageOverlay> overlays)
    : string_pool_(pool),
      row_count_(row_count),
      overlays_(std::move(overlays)),
      columns_(std::move(columns)) {
  PERFETTO_DCHECK(string_pool_);
}

Table::~Table() = default;

Table& Table::operator=(Table&& other) noexcept {
  row_count_ = other.row_count_;
  string_pool_ = other.string_pool_;

  overlays_ = std::move(other.overlays_);
  columns_ = std::move(other.columns_);

  storage_layers_ = std::move(other.storage_layers_);
  null_layers_ = std::move(other.null_layers_);
  overlay_layers_ = std::move(other.overlay_layers_);
  chains_ = std::move(other.chains_);

  for (ColumnLegacy& col : columns_) {
    col.table_ = this;
  }
  return *this;
}

Table Table::Copy() const {
  Table table = CopyExceptOverlays();
  for (const ColumnStorageOverlay& overlay : overlays_) {
    table.overlays_.emplace_back(overlay.Copy());
  }
  table.OnConstructionCompleted(storage_layers_, null_layers_, overlay_layers_);
  return table;
}

Table Table::CopyExceptOverlays() const {
  std::vector<ColumnLegacy> cols;
  cols.reserve(columns_.size());
  for (const ColumnLegacy& col : columns_) {
    cols.emplace_back(col, col.index_in_table(), col.overlay_index());
  }
  return {string_pool_, row_count_, std::move(cols), {}};
}

RowMap Table::QueryToRowMap(const std::vector<Constraint>& cs,
                            const std::vector<Order>& ob,
                            RowMap::OptimizeFor optimize_for) const {
  RowMap rm = FilterToRowMap(cs, optimize_for);
  if (ob.empty())
    return rm;

  // Return the RowMap directly if there is a single constraint to sort the
  // table by a column which is already sorted.
  const auto& first_col = columns_[ob.front().col_idx];
  if (ob.size() == 1 && first_col.IsSorted() && !ob.front().desc)
    return rm;

  // Build an index vector with all the indices for the first |size_| rows.
  std::vector<uint32_t> idx = std::move(rm).TakeAsIndexVector();
  if (ob.size() == 1 && first_col.IsSorted()) {
    // We special case a single constraint in descending order as this
    // happens any time the |max| function is used in SQLite. We can be
    // more efficient as this column is already sorted so we simply need
    // to reverse the order of this column.
    PERFETTO_DCHECK(ob.front().desc);
    std::reverse(idx.begin(), idx.end());
  } else {
    // As our data is columnar, it's always more efficient to sort one column
    // at a time rather than try and sort lexiographically all at once.
    // To preserve correctness, we need to stably sort the index vector once
    // for each order by in *reverse* order. Reverse order is important as it
    // preserves the lexiographical property.
    //
    // For example, suppose we have the following:
    // Table {
    //   Column x;
    //   Column y
    //   Column z;
    // }
    //
    // Then, to sort "y asc, x desc", we could do one of two things:
    //  1) sort the index vector all at once and on each index, we compare
    //     y then z. This is slow as the data is columnar and we need to
    //     repeatedly branch inside each column.
    //  2) we can stably sort first on x desc and then sort on y asc. This will
    //     first put all the x in the correct order such that when we sort on
    //     y asc, we will have the correct order of x where y is the same (since
    //     the sort is stable).
    //
    // TODO(lalitm): it is possible that we could sort the last constraint (i.e.
    // the first constraint in the below loop) in a non-stable way. However,
    // this is more subtle than it appears as we would then need special
    // handling where there are order bys on a column which is already sorted
    // (e.g. ts, id). Investigate whether the performance gains from this are
    // worthwhile. This also needs changes to the constraint modification logic
    // in DbSqliteTable which currently eliminates constraints on sorted
    // columns.
    for (auto it = ob.rbegin(); it != ob.rend(); ++it) {
      columns_[it->col_idx].StableSort(it->desc, &idx);
    }
  }
  return RowMap(std::move(idx));
}

Table Table::Sort(const std::vector<Order>& ob) const {
  if (ob.empty()) {
    return Copy();
  }

  // Return a copy of this table with the RowMaps using the computed ordered
  // RowMap.
  Table table = CopyExceptOverlays();
  RowMap rm = QueryToRowMap({}, ob);
  for (const ColumnStorageOverlay& overlay : overlays_) {
    table.overlays_.emplace_back(overlay.SelectRows(rm));
    PERFETTO_DCHECK(table.overlays_.back().size() == table.row_count());
  }
  table.OnConstructionCompleted(storage_layers_, null_layers_, overlay_layers_);

  // Remove the sorted and row set flags from all the columns.
  for (auto& col : table.columns_) {
    col.flags_ &= ~ColumnLegacy::Flag::kSorted;
    col.flags_ &= ~ColumnLegacy::Flag::kSetId;
  }

  // For the first order by, make the column flag itself as sorted but
  // only if the sort was in ascending order.
  if (!ob.front().desc) {
    table.columns_[ob.front().col_idx].flags_ |= ColumnLegacy::Flag::kSorted;
  }
  return table;
}

}  // namespace perfetto::trace_processor
