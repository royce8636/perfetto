/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <array>
#include <numeric>
#include <vector>

#include "src/trace_processor/db/query_executor.h"
#include "src/trace_processor/db/table.h"

namespace perfetto {
namespace trace_processor {

namespace {

using Range = RowMap::Range;
using OverlayOp = overlays::OverlayOp;
using StorageRange = overlays::StorageRange;
using TableRange = overlays::TableRange;
using Storage = storage::Storage;
using StorageOverlay = overlays::StorageOverlay;
using TableIndexVector = overlays::TableIndexVector;
using StorageIndexVector = overlays::StorageIndexVector;
using TableBitVector = overlays::TableBitVector;
using StorageBitVector = overlays::StorageBitVector;

// Helper struct to simplify operations on |global| and |current| sets of
// indices. Having this coupling enables efficient implementation of
// IndexedColumnFilter.
struct IndexFilterHelper {
  explicit IndexFilterHelper(std::vector<uint32_t> indices)
      : IndexFilterHelper(indices, std::move(indices)) {}

  // Removes pairs of elements that are not set in the |bv| and returns
  // Indices made of them.
  static std::pair<IndexFilterHelper, IndexFilterHelper> Partition(
      IndexFilterHelper indices,
      const BitVector& bv) {
    if (bv.CountSetBits() == 0) {
      return {IndexFilterHelper(), indices};
    }

    IndexFilterHelper set_partition;
    IndexFilterHelper non_set_partition;
    for (auto it = bv.IterateAllBits(); it; it.Next()) {
      uint32_t idx = it.index();
      if (it.IsSet()) {
        set_partition.PushBack({indices.current_[idx], indices.global_[idx]});
      } else {
        non_set_partition.PushBack(
            {indices.current_[idx], indices.global_[idx]});
      }
    }
    return {set_partition, non_set_partition};
  }

  // Removes pairs of elements that are not set in the |bv|. Returns count of
  // removed elements.
  uint32_t KeepAtSet(BitVector filter_nulls) {
    PERFETTO_CHECK(filter_nulls.size() == current_.size() ||
                   filter_nulls.CountSetBits() == 0);
    uint32_t count_removed =
        static_cast<uint32_t>(current_.size()) - filter_nulls.CountSetBits();

    uint32_t i = 0;
    auto filter = [&i, &filter_nulls](uint32_t) {
      return !filter_nulls.IsSet(i++);
    };

    auto current_it = std::remove_if(current_.begin(), current_.end(), filter);
    current_.erase(current_it, current_.end());

    i = 0;
    auto global_it = std::remove_if(global_.begin(), global_.end(), filter);
    global_.erase(global_it, global_.end());

    return count_removed;
  }

  std::vector<uint32_t>& current() { return current_; }

  std::vector<uint32_t>& global() { return global_; }

 private:
  IndexFilterHelper() = default;
  IndexFilterHelper(std::vector<uint32_t> current, std::vector<uint32_t> global)
      : current_(std::move(current)), global_(std::move(global)) {
    PERFETTO_CHECK(current_.size() == global_.size());
  }

  void PushBack(std::pair<uint32_t, uint32_t> cur_and_global_idx) {
    current_.push_back(cur_and_global_idx.first);
    global_.push_back(cur_and_global_idx.second);
  }

  std::vector<uint32_t> current_;
  std::vector<uint32_t> global_;
};
}  // namespace

void QueryExecutor::FilterColumn(const Constraint& c,
                                 const SimpleColumn& col,
                                 RowMap* rm) {
  uint32_t rm_first = rm->Get(0);
  uint32_t rm_last = rm->Get(rm->size() - 1);
  uint32_t range_size = rm_last - rm_first;
  // If the range is less than 50% full and size() < 1024, choose the index
  // algorithm.
  // TODO(b/283763282):Use Overlay estimations.
  if (rm->size() < 1024 &&
      (static_cast<double>(rm->size()) / range_size < 0.5)) {
    *rm = IndexedColumnFilter(c, col, rm);
    return;
  }
  rm->Intersect(BoundedColumnFilter(c, col, rm));
}

RowMap QueryExecutor::BoundedColumnFilter(const Constraint& c,
                                          const SimpleColumn& col,
                                          RowMap* rm) {
  // TODO(b/283763282): We should align these to word boundaries.
  TableRange table_range{Range(rm->Get(0), rm->Get(rm->size() - 1) + 1)};
  base::SmallVector<Range, kMaxOverlayCount> overlay_bounds;

  for (const auto& overlay : col.overlays) {
    StorageRange storage_range = overlay->MapToStorageRange(table_range);
    overlay_bounds.emplace_back(storage_range.range);
    table_range = TableRange({storage_range.range});
  }

  // Use linear search algorithm on storage.
  overlays::StorageBitVector filtered_storage{
      col.storage->LinearSearch(c.op, c.value, table_range.range)};

  for (uint32_t i = 0; i < col.overlays.size(); ++i) {
    uint32_t rev_i = static_cast<uint32_t>(col.overlays.size()) - 1 - i;
    TableBitVector mapped_to_table = col.overlays[rev_i]->MapToTableBitVector(
        std::move(filtered_storage), overlays::FilterOpToOverlayOp(c.op));
    filtered_storage = StorageBitVector({std::move(mapped_to_table.bv)});
  }
  return RowMap(std::move(filtered_storage.bv));
}

RowMap QueryExecutor::IndexedColumnFilter(const Constraint& c,
                                          const SimpleColumn& col,
                                          RowMap* rm) {
  // Create outmost TableIndexVector.
  std::vector<uint32_t> table_indices;
  table_indices.reserve(rm->size());
  for (auto it = rm->IterateRows(); it; it.Next()) {
    table_indices.push_back(it.index());
  }

  // Datastructures for storing data across overlays.
  IndexFilterHelper to_filter(std::move(table_indices));
  std::vector<uint32_t> valid;
  uint32_t count_removed = 0;

  // Fetch the list of indices that require storage lookup and deal with all
  // of the indices that can be compared before it.
  OverlayOp op = overlays::FilterOpToOverlayOp(c.op);
  for (auto overlay : col.overlays) {
    BitVector partition =
        overlay->IsStorageLookupRequired(op, {to_filter.current()});

    // Most overlays don't require partitioning.
    if (partition.CountSetBits() == partition.size()) {
      to_filter.current() =
          overlay->MapToStorageIndexVector({to_filter.current()}).indices;
      continue;
    }

    // Separate indices that don't require storage lookup. Those can be dealt
    // with in each pass.
    auto [storage_lookup, no_storage_lookup] =
        IndexFilterHelper::Partition(to_filter, partition);
    to_filter = storage_lookup;

    // Erase the values which don't match the constraint and add the
    // remaining ones to the result.
    BitVector valid_bv =
        overlay->IndexSearch(op, {no_storage_lookup.current()});
    count_removed += no_storage_lookup.KeepAtSet(std::move(valid_bv));
    valid.insert(valid.end(), no_storage_lookup.global().begin(),
                 no_storage_lookup.global().end());

    // Update the current indices to the next storage overlay.
    to_filter.current() =
        overlay->MapToStorageIndexVector({to_filter.current()}).indices;
  }

  BitVector matched_in_storage = col.storage->IndexSearch(
      c.op, c.value, to_filter.current().data(),
      static_cast<uint32_t>(to_filter.current().size()));
  count_removed += to_filter.KeepAtSet(std::move(matched_in_storage));
  valid.insert(valid.end(), to_filter.global().begin(),
               to_filter.global().end());

  PERFETTO_CHECK(rm->size() == valid.size() + count_removed);

  std::sort(valid.begin(), valid.end());
  return RowMap(std::move(valid));
}

}  // namespace trace_processor
}  // namespace perfetto
