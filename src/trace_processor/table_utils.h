/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef SRC_TRACE_PROCESSOR_TABLE_UTILS_H_
#define SRC_TRACE_PROCESSOR_TABLE_UTILS_H_

#include <memory>
#include <set>

#include "src/trace_processor/row_iterators.h"
#include "src/trace_processor/storage_schema.h"

namespace perfetto {
namespace trace_processor {
namespace table_utils {

namespace internal {

inline FilteredRowIndex CreateRangeIterator(
    const StorageSchema& schema,
    uint32_t size,
    const std::vector<QueryConstraints::Constraint>& cs,
    sqlite3_value** argv) {
  // Try and bound the search space to the smallest possible index region and
  // store any leftover constraints to filter using bitvector.
  uint32_t min_idx = 0;
  uint32_t max_idx = size;
  std::vector<size_t> bitvector_cs;
  for (size_t i = 0; i < cs.size(); i++) {
    const auto& c = cs[i];
    size_t column = static_cast<size_t>(c.iColumn);
    auto bounds = schema.GetColumn(column).BoundFilter(c.op, argv[i]);

    min_idx = std::max(min_idx, bounds.min_idx);
    max_idx = std::min(max_idx, bounds.max_idx);

    // If the lower bound is higher than the upper bound, return a zero-sized
    // range iterator.
    if (min_idx >= max_idx)
      return FilteredRowIndex(min_idx, min_idx);

    if (!bounds.consumed)
      bitvector_cs.emplace_back(i);
  }

  // Create an filter index and allow each of the columns filter on it.
  FilteredRowIndex index(min_idx, max_idx);
  for (const auto& c_idx : bitvector_cs) {
    const auto& c = cs[c_idx];
    auto* value = argv[c_idx];

    const auto& schema_col = schema.GetColumn(static_cast<size_t>(c.iColumn));
    schema_col.Filter(c.op, value, &index);
  }
  return index;
}

inline std::pair<bool, bool> IsOrdered(
    const StorageSchema& schema,
    const std::vector<QueryConstraints::OrderBy>& obs) {
  if (obs.size() == 0)
    return std::make_pair(true, false);

  if (obs.size() != 1)
    return std::make_pair(false, false);

  const auto& ob = obs[0];
  auto col = static_cast<size_t>(ob.iColumn);
  return std::make_pair(schema.GetColumn(col).IsNaturallyOrdered(), ob.desc);
}

inline std::vector<QueryConstraints::OrderBy> RemoveRedundantOrderBy(
    const std::vector<QueryConstraints::Constraint>& cs,
    const std::vector<QueryConstraints::OrderBy>& obs) {
  std::vector<QueryConstraints::OrderBy> filtered;
  std::set<int> equality_cols;
  for (const auto& c : cs) {
    if (sqlite_utils::IsOpEq(c.op))
      equality_cols.emplace(c.iColumn);
  }
  for (const auto& o : obs) {
    if (equality_cols.count(o.iColumn) > 0)
      continue;
    filtered.emplace_back(o);
  }
  return filtered;
}

inline std::vector<uint32_t> CreateSortedIndexVector(
    const StorageSchema& schema,
    FilteredRowIndex index,
    const std::vector<QueryConstraints::OrderBy>& obs) {
  PERFETTO_DCHECK(obs.size() > 0);

  // Retrieve the index created above from the index.
  std::vector<uint32_t> sorted_rows = index.ToRowVector();

  std::vector<StorageColumn::Comparator> comparators;
  for (const auto& ob : obs) {
    auto col = static_cast<size_t>(ob.iColumn);
    comparators.emplace_back(schema.GetColumn(col).Sort(ob));
  }

  auto comparator = [&comparators](uint32_t f, uint32_t s) {
    for (const auto& comp : comparators) {
      int c = comp(f, s);
      if (c != 0)
        return c < 0;
    }
    return false;
  };
  std::sort(sorted_rows.begin(), sorted_rows.end(), comparator);

  return sorted_rows;
}

}  // namespace internal

// Creates a row iterator which is optimized for a generic storage schema (i.e.
// it does not make assumptions about values of columns).
inline std::unique_ptr<RowIterator> CreateBestRowIteratorForGenericSchema(
    const StorageSchema& schema,
    uint32_t size,
    const QueryConstraints& qc,
    sqlite3_value** argv) {
  const auto& cs = qc.constraints();
  auto obs = internal::RemoveRedundantOrderBy(cs, qc.order_by());

  // Figure out whether the data is already ordered and which order we should
  // traverse the data.
  bool is_ordered, desc = false;
  std::tie(is_ordered, desc) = internal::IsOrdered(schema, obs);

  // Create the range iterator and if we are sorted, just return it.
  auto index = internal::CreateRangeIterator(schema, size, cs, argv);
  if (is_ordered) {
    return index.ToRowIterator(desc);
  }
  // Otherwise, create the sorted vector of indices and create the vector
  // iterator.
  return std::unique_ptr<VectorRowIterator>(new VectorRowIterator(
      internal::CreateSortedIndexVector(schema, std::move(index), obs)));
}

}  // namespace table_utils
}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_TABLE_UTILS_H_
