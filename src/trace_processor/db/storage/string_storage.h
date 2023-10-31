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
#ifndef SRC_TRACE_PROCESSOR_DB_STORAGE_STRING_STORAGE_H_
#define SRC_TRACE_PROCESSOR_DB_STORAGE_STRING_STORAGE_H_

#include <variant>

#include "src/trace_processor/containers/row_map.h"
#include "src/trace_processor/containers/string_pool.h"
#include "src/trace_processor/db/storage/storage.h"
#include "src/trace_processor/db/storage/types.h"

namespace perfetto {
namespace trace_processor {
namespace storage {

// Storage for String columns.
class StringStorage final : public Storage {
 public:
  StringStorage(StringPool* string_pool,
                const StringPool::Id* data,
                uint32_t data_size,
                bool is_sorted = false)
      : data_(data),
        size_(data_size),
        string_pool_(string_pool),
        is_sorted_(is_sorted) {}

  RangeOrBitVector Search(FilterOp op,
                          SqlValue value,
                          RowMap::Range range) const override;

  RangeOrBitVector IndexSearch(FilterOp op,
                               SqlValue value,
                               uint32_t* indices,
                               uint32_t indices_count,
                               bool sorted = false) const override;
  void StableSort(uint32_t* rows, uint32_t rows_size) const override;

  void Sort(uint32_t* rows, uint32_t rows_size) const override;

  uint32_t size() const override { return size_; }

 private:
  BitVector LinearSearchInternal(FilterOp, SqlValue, RowMap::Range) const;

  RangeOrBitVector IndexSearchInternal(FilterOp op,
                                       SqlValue sql_val,
                                       uint32_t* indices,
                                       uint32_t indices_size,
                                       bool) const;

  RowMap::Range BinarySearchExtrinsic(FilterOp,
                                      SqlValue,
                                      uint32_t*,
                                      uint32_t) const;

  RowMap::Range BinarySearchIntrinsic(FilterOp op,
                                      SqlValue val,
                                      RowMap::Range search_range) const;

  const StringPool::Id* data_ = nullptr;
  const uint32_t size_ = 0;

  StringPool* string_pool_ = nullptr;
  const bool is_sorted_ = false;
};

}  // namespace storage
}  // namespace trace_processor
}  // namespace perfetto
#endif  // SRC_TRACE_PROCESSOR_DB_STORAGE_STRING_STORAGE_H_
