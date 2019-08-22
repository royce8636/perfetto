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

#ifndef SRC_TRACE_PROCESSOR_DB_TABLE_H_
#define SRC_TRACE_PROCESSOR_DB_TABLE_H_

#include <stdint.h>

#include <limits>
#include <numeric>
#include <vector>

#include "perfetto/base/logging.h"
#include "perfetto/ext/base/optional.h"
#include "src/trace_processor/db/column.h"

namespace perfetto {
namespace trace_processor {

// Represents a table of data with named, strongly typed columns.
class Table {
 public:
  // Iterator over the rows of the table.
  class Iterator {
   public:
    Iterator(const Table* table) : table_(table) {}

    bool Next() { return ++row_ < table_->size(); }

    // Returns the value at the current row for column |col_idx|.
    base::Optional<int64_t> Get(uint32_t col_idx) {
      return table_->columns_[col_idx].Get(row_);
    }

   private:
    const Table* table_ = nullptr;
    uint32_t row_ = std::numeric_limits<uint32_t>::max();
  };

  // We explicitly define the move constructor here because we need to update
  // the Table pointer in each column in the table.
  Table(Table&& other) noexcept { *this = std::move(other); }
  Table& operator=(Table&& other) noexcept;

  // Filters the Table using the specified filter constraints.
  Table Filter(const std::vector<Constraint>& cs) const;

  // Sorts the Table using the specified order by constraints.
  Table Sort(const std::vector<Order>& od) const;

  // Joins |this| table with the |other| table using the values of column |left|
  // of |this| table to lookup the row in |right| column of the |other| table.
  //
  // Concretely, for each row in the returned table we lookup the value of
  // |left| in |right|. The found row is used as the values for |other|'s
  // columns in the returned table.
  //
  // This means we obtain the following invariants:
  //  1. this->size() == ret->size()
  //  2. this->Rows()[i].Get(j) == ret->Rows()[i].Get(j)
  //
  // It also means there are few restrictions on the data in |left| and |right|:
  //  * |left| is not allowed to have any nulls.
  //  * |left|'s values must exist in |right|
  Table LookupJoin(JoinKey left, const Table& other, JoinKey right);

  // Returns the name of the column at index |idx| in the Table.
  const char* GetColumnName(uint32_t idx) const { return columns_[idx].name(); }

  // Returns the number of columns in the Table.
  uint32_t GetColumnCount() const {
    return static_cast<uint32_t>(columns_.size());
  }

  // Returns an iterator into the Table.
  Iterator IterateRows() const { return Iterator(this); }

  uint32_t size() const { return size_; }
  const std::vector<RowMap>& row_maps() const { return row_maps_; }

 protected:
  explicit Table(const Table* parent);

  std::vector<RowMap> row_maps_;
  std::vector<Column> columns_;
  uint32_t size_ = 0;

 private:
  friend class Column;

  // We explicitly define the copy constructor here because we need to change
  // the Table pointer in each column to the Table being copied into.
  Table(const Table& other) { *this = other; }
  Table& operator=(const Table& other);
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_DB_TABLE_H_
