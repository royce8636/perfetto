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
#ifndef SRC_TRACE_PROCESSOR_DB_STORAGE_H_
#define SRC_TRACE_PROCESSOR_DB_STORAGE_H_

#include <variant>
#include "perfetto/ext/base/status_or.h"
#include "src/trace_processor/db/column.h"

namespace perfetto {
namespace trace_processor {
namespace column {

// Most base column interpreting layer - responsible for implementing operations
// that require looking at the data, such as comparison or sorting.
class Storage {
 public:
  virtual ~Storage();

  // Changes the vector of indices to represent the sorted state of the column.
  virtual void StableSort(std::vector<uint32_t>&) const = 0;

  // Efficiently compares series of |num_elements| of data from |data_start| to
  // comparator value and appends results to BitVector::Builder. Should be used
  // on as much data as possible.
  virtual void CompareFast(FilterOp op,
                           SqlValue value,
                           const void* start,
                           uint32_t compare_elements_count,
                           BitVector::Builder&) const = 0;

  // Inefficiently compares series of |num_elements| of data from |data_start|
  // to comparator value and appends results to BitVector::Builder. Should be
  // avoided if possible, with `FastSeriesComparison` used instead.
  virtual void CompareSlow(FilterOp op,
                           SqlValue value,
                           const void* data_start,
                           uint32_t compare_elements_count,
                           BitVector::Builder&) const = 0;

  // Compares sorted (asc) series of |num_elements| of data from |data_start| to
  // comparator value. Should be used where possible.
  virtual void CompareSorted(FilterOp op,
                             SqlValue value,
                             const void* data_start,
                             uint32_t compare_elements_count,
                             RowMap&) const = 0;
};

}  // namespace column
}  // namespace trace_processor
}  // namespace perfetto
#endif  // SRC_TRACE_PROCESSOR_DB_STORAGE_H_
