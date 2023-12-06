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

#include "src/trace_processor/db/storage/null_storage.h"

#include <cstdint>
#include <variant>

#include "protos/perfetto/trace_processor/serialization.pbzero.h"
#include "src/trace_processor/containers/bit_vector.h"
#include "src/trace_processor/containers/row_map.h"
#include "src/trace_processor/db/storage/types.h"
#include "src/trace_processor/tp_metatrace.h"

namespace perfetto {
namespace trace_processor {
namespace storage {
namespace {

using Range = RowMap::Range;

RangeOrBitVector ReconcileStorageResult(FilterOp op,
                                        const BitVector& non_null,
                                        RangeOrBitVector storage_result,
                                        Range in_range) {
  PERFETTO_CHECK(in_range.end <= non_null.size());

  // Reconcile the results of the Search operation with the non-null indices
  // to ensure only those positions are set.
  BitVector res;
  if (storage_result.IsRange()) {
    Range range = std::move(storage_result).TakeIfRange();
    if (range.size() > 0) {
      res = non_null.IntersectRange(non_null.IndexOfNthSet(range.start),
                                    non_null.IndexOfNthSet(range.end - 1) + 1);

      // We should always have at least as many elements as the input range
      // itself.
      PERFETTO_CHECK(res.size() <= in_range.end);
    }
  } else {
    res = non_null.Copy();
    res.UpdateSetBits(std::move(storage_result).TakeIfBitVector());
  }

  // Ensure that |res| exactly matches the size which we need to return,
  // padding with zeros or truncating if necessary.
  res.Resize(in_range.end, false);

  // For the IS NULL constraint, we also need to include all the null indices
  // themselves.
  if (PERFETTO_UNLIKELY(op == FilterOp::kIsNull)) {
    BitVector null = non_null.IntersectRange(in_range.start, in_range.end);
    null.Resize(in_range.end, false);
    null.Not();
    res.Or(null);
  }
  return RangeOrBitVector(std::move(res));
}

}  // namespace

NullStorage::NullStorage(std::unique_ptr<Storage> storage,
                         const BitVector* non_null)
    : storage_(std::move(storage)), non_null_(non_null) {
  PERFETTO_DCHECK(non_null_->CountSetBits() <= storage_->size());
}

RangeOrBitVector NullStorage::Search(FilterOp op,
                                     SqlValue sql_val,
                                     RowMap::Range in) const {
  PERFETTO_TP_TRACE(metatrace::Category::DB, "NullStorage::Search");

  // Figure out the bounds of the indices in the underlying storage and search
  // it.
  uint32_t start = non_null_->CountSetBits(in.start);
  uint32_t end = non_null_->CountSetBits(in.end);
  return ReconcileStorageResult(
      op, *non_null_, storage_->Search(op, sql_val, RowMap::Range(start, end)),
      in);
}

RangeOrBitVector NullStorage::IndexSearch(FilterOp op,
                                          SqlValue sql_val,
                                          uint32_t* indices,
                                          uint32_t indices_size,
                                          bool sorted) const {
  PERFETTO_TP_TRACE(metatrace::Category::DB, "NullStorage::IndexSearch");

  BitVector::Builder storage_non_null(indices_size);
  std::vector<uint32_t> storage_iv;
  storage_iv.reserve(indices_size);
  for (uint32_t* it = indices; it != indices + indices_size; it++) {
    bool is_non_null = non_null_->IsSet(*it);
    if (is_non_null) {
      storage_iv.push_back(non_null_->CountSetBits(*it));
    }
    storage_non_null.Append(is_non_null);
  }
  RangeOrBitVector range_or_bv =
      storage_->IndexSearch(op, sql_val, storage_iv.data(),
                            static_cast<uint32_t>(storage_iv.size()), sorted);
  return ReconcileStorageResult(op, std::move(storage_non_null).Build(),
                                std::move(range_or_bv), Range(0, indices_size));
}

void NullStorage::StableSort(uint32_t*, uint32_t) const {
  // TODO(b/307482437): Implement.
  PERFETTO_FATAL("Not implemented");
}

void NullStorage::Sort(uint32_t*, uint32_t) const {
  // TODO(b/307482437): Implement.
  PERFETTO_FATAL("Not implemented");
}

void NullStorage::Serialize(StorageProto* storage) const {
  auto* null_storage = storage->set_null_storage();
  non_null_->Serialize(null_storage->set_bit_vector());
  storage_->Serialize(null_storage->set_storage());
}

}  // namespace storage
}  // namespace trace_processor
}  // namespace perfetto
