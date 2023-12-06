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

#include "src/trace_processor/db/storage/arrangement_storage.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "protos/perfetto/trace_processor/serialization.pbzero.h"
#include "src/trace_processor/containers/bit_vector.h"
#include "src/trace_processor/tp_metatrace.h"

namespace perfetto {
namespace trace_processor {
namespace storage {
namespace {

using Range = RowMap::Range;

}  // namespace

ArrangementStorage::ArrangementStorage(std::unique_ptr<Storage> inner,
                                       const std::vector<uint32_t>* arrangement)
    : inner_(std::move(inner)), arrangement_(arrangement) {
  PERFETTO_DCHECK(*std::max_element(arrangement->begin(), arrangement->end()) <=
                  inner_->size());
}

RangeOrBitVector ArrangementStorage::Search(FilterOp op,
                                            SqlValue sql_val,
                                            Range in) const {
  PERFETTO_TP_TRACE(metatrace::Category::DB, "ArrangementStorage::Search");

  const auto& arrangement = *arrangement_;
  PERFETTO_DCHECK(in.end <= arrangement.size());
  const auto [min_i, max_i] =
      std::minmax_element(arrangement.begin() + static_cast<int32_t>(in.start),
                          arrangement.begin() + static_cast<int32_t>(in.end));

  auto storage_result = inner_->Search(op, sql_val, Range(*min_i, *max_i + 1));
  BitVector::Builder builder(in.end, in.start);
  if (storage_result.IsRange()) {
    Range storage_range = std::move(storage_result).TakeIfRange();
    for (uint32_t i = in.start; i < in.end; ++i) {
      builder.Append(storage_range.Contains(arrangement[i]));
    }
  } else {
    BitVector storage_bitvector = std::move(storage_result).TakeIfBitVector();

    // After benchmarking, it turns out this complexity *is* actually worthwhile
    // and has a noticable impact on the performance of this function in real
    // world tables.

    // Fast path: we compare as many groups of 64 elements as we can.
    // This should be very easy for the compiler to auto-vectorize.
    uint32_t fast_path_elements = builder.BitsInCompleteWordsUntilFull();
    uint32_t cur_idx = 0;
    for (uint32_t i = 0; i < fast_path_elements; i += BitVector::kBitsInWord) {
      uint64_t word = 0;
      // This part should be optimised by SIMD and is expected to be fast.
      for (uint32_t k = 0; k < BitVector::kBitsInWord; ++k, ++cur_idx) {
        bool comp_result = storage_bitvector.IsSet((*arrangement_)[cur_idx]);
        word |= static_cast<uint64_t>(comp_result) << k;
      }
      builder.AppendWord(word);
    }

    // Slow path: we compare <64 elements and append to fill the Builder.
    uint32_t back_elements = builder.BitsUntilFull();
    for (uint32_t i = 0; i < back_elements; ++i, ++cur_idx) {
      builder.Append(storage_bitvector.IsSet((*arrangement_)[cur_idx]));
    }
  }
  return RangeOrBitVector(std::move(builder).Build());
}

RangeOrBitVector ArrangementStorage::IndexSearch(FilterOp op,
                                                 SqlValue sql_val,
                                                 uint32_t* indices,
                                                 uint32_t indices_size,
                                                 bool sorted) const {
  PERFETTO_TP_TRACE(metatrace::Category::DB, "ArrangementStorage::IndexSearch");

  std::vector<uint32_t> storage_iv;
  for (uint32_t* it = indices; it != indices + indices_size; ++it) {
    storage_iv.push_back((*arrangement_)[*it]);
  }
  return inner_->IndexSearch(op, sql_val, storage_iv.data(),
                             static_cast<uint32_t>(storage_iv.size()), sorted);
}

void ArrangementStorage::StableSort(uint32_t*, uint32_t) const {
  // TODO(b/307482437): Implement.
  PERFETTO_FATAL("Not implemented");
}

void ArrangementStorage::Sort(uint32_t*, uint32_t) const {
  // TODO(b/307482437): Implement.
  PERFETTO_FATAL("Not implemented");
}

void ArrangementStorage::Serialize(StorageProto* storage) const {
  auto* arrangement_storage = storage->set_arrangement_storage();
  arrangement_storage->set_values(
      reinterpret_cast<const uint8_t*>(arrangement_->data()),
      sizeof(uint32_t) * arrangement_->size());
  inner_->Serialize(arrangement_storage->set_storage());
}

}  // namespace storage
}  // namespace trace_processor
}  // namespace perfetto
