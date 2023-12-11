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
#include "src/trace_processor/db/storage/set_id_storage.h"

#include "test/gtest_and_gmock.h"

namespace perfetto {
namespace trace_processor {

inline bool operator==(const RowMap::Range& a, const RowMap::Range& b) {
  return std::tie(a.start, a.end) == std::tie(b.start, b.end);
}

namespace storage {
namespace {

using Range = RowMap::Range;

TEST(SetIdStorageUnittest, InvalidSearchConstraints) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);
  // NULL checks
  ASSERT_EQ(storage.ValidateSearchConstraints(SqlValue(), FilterOp::kIsNull),
            SearchValidationResult::kNoData);
  ASSERT_EQ(storage.ValidateSearchConstraints(SqlValue(), FilterOp::kIsNotNull),
            SearchValidationResult::kAllData);

  // FilterOp checks
  ASSERT_EQ(
      storage.ValidateSearchConstraints(SqlValue::Long(15), FilterOp::kGlob),
      SearchValidationResult::kNoData);
  ASSERT_EQ(
      storage.ValidateSearchConstraints(SqlValue::Long(15), FilterOp::kRegex),
      SearchValidationResult::kNoData);

  // Type checks
  ASSERT_EQ(storage.ValidateSearchConstraints(SqlValue::String("cheese"),
                                              FilterOp::kGe),
            SearchValidationResult::kNoData);

  // Value bounds
  SqlValue max_val = SqlValue::Long(
      static_cast<int64_t>(std::numeric_limits<uint32_t>::max()) + 10);
  ASSERT_EQ(storage.ValidateSearchConstraints(max_val, FilterOp::kGe),
            SearchValidationResult::kNoData);
  ASSERT_EQ(storage.ValidateSearchConstraints(max_val, FilterOp::kGt),
            SearchValidationResult::kNoData);
  ASSERT_EQ(storage.ValidateSearchConstraints(max_val, FilterOp::kEq),
            SearchValidationResult::kNoData);

  ASSERT_EQ(storage.ValidateSearchConstraints(max_val, FilterOp::kLe),
            SearchValidationResult::kAllData);
  ASSERT_EQ(storage.ValidateSearchConstraints(max_val, FilterOp::kLt),
            SearchValidationResult::kAllData);
  ASSERT_EQ(storage.ValidateSearchConstraints(max_val, FilterOp::kNe),
            SearchValidationResult::kAllData);

  SqlValue min_val = SqlValue::Long(
      static_cast<int64_t>(std::numeric_limits<uint32_t>::min()) - 1);
  ASSERT_EQ(storage.ValidateSearchConstraints(min_val, FilterOp::kGe),
            SearchValidationResult::kAllData);
  ASSERT_EQ(storage.ValidateSearchConstraints(min_val, FilterOp::kGt),
            SearchValidationResult::kAllData);
  ASSERT_EQ(storage.ValidateSearchConstraints(min_val, FilterOp::kNe),
            SearchValidationResult::kAllData);

  ASSERT_EQ(storage.ValidateSearchConstraints(min_val, FilterOp::kLe),
            SearchValidationResult::kNoData);
  ASSERT_EQ(storage.ValidateSearchConstraints(min_val, FilterOp::kLt),
            SearchValidationResult::kNoData);
  ASSERT_EQ(storage.ValidateSearchConstraints(min_val, FilterOp::kEq),
            SearchValidationResult::kNoData);
}

TEST(SetIdStorageUnittest, SearchEqSimple) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kEq, SqlValue::Long(3), Range(4, 10))
                    .TakeIfRange();

  ASSERT_EQ(range.size(), 2u);
  ASSERT_EQ(range.start, 4u);
  ASSERT_EQ(range.end, 6u);
}

TEST(SetIdStorageUnittest, SearchEqOnRangeBoundary) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kEq, SqlValue::Long(9), Range(6, 9))
                    .TakeIfRange();
  ASSERT_EQ(range.size(), 0u);
}

TEST(SetIdStorageUnittest, SearchEqOutsideRange) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kEq, SqlValue::Long(12), Range(6, 9))
                    .TakeIfRange();
  ASSERT_EQ(range.size(), 0u);
}

TEST(SetIdStorageUnittest, SearchEqTooBig) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kEq, SqlValue::Long(100), Range(6, 9))
                    .TakeIfRange();
  ASSERT_EQ(range.size(), 0u);
}

TEST(SetIdStorageUnittest, SearchNe) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  BitVector bv = storage.Search(FilterOp::kNe, SqlValue::Long(3), Range(1, 7))
                     .TakeIfBitVector();
  ASSERT_EQ(bv.CountSetBits(), 3u);
}

TEST(SetIdStorageUnittest, SearchLe) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kLe, SqlValue::Long(3), Range(1, 7))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 1u);
  ASSERT_EQ(range.end, 6u);
}

TEST(SetIdStorageUnittest, SearchLt) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kLt, SqlValue::Long(3), Range(1, 7))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 1u);
  ASSERT_EQ(range.end, 3u);
}

TEST(SetIdStorageUnittest, SearchGe) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kGe, SqlValue::Long(6), Range(5, 10))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 6u);
  ASSERT_EQ(range.end, 10u);
}

TEST(SetIdStorageUnittest, SearchGt) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};

  SetIdStorage storage(&storage_data);
  Range range = storage.Search(FilterOp::kGt, SqlValue::Long(6), Range(5, 10))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 9u);
  ASSERT_EQ(range.end, 10u);
}

TEST(SetIdStorageUnittest, IndexSearchEqSimple) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);

  // {0, 3, 3, 6, 9, 9, 0, 3}
  std::vector<uint32_t> indices{1, 3, 5, 7, 9, 11, 2, 4};

  BitVector bv = storage
                     .IndexSearch(FilterOp::kEq, SqlValue::Long(3),
                                  indices.data(), 8, false)
                     .TakeIfBitVector();

  ASSERT_EQ(bv.CountSetBits(), 3u);
  ASSERT_TRUE(bv.IsSet(1));
  ASSERT_TRUE(bv.IsSet(2));
  ASSERT_TRUE(bv.IsSet(7));
}

TEST(SetIdStorageUnittest, IndexSearchEqTooBig) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);

  // {0, 3, 3, 6, 9, 9, 0, 3}
  std::vector<uint32_t> indices{1, 3, 5, 7, 9, 11, 2, 4};

  BitVector bv = storage
                     .IndexSearch(FilterOp::kEq, SqlValue::Long(10),
                                  indices.data(), 8, false)
                     .TakeIfBitVector();

  ASSERT_EQ(bv.CountSetBits(), 0u);
}

TEST(SetIdStorageUnittest, IndexSearchNe) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);

  // {0, 3, 3, 6, 9, 9, 0, 3}
  std::vector<uint32_t> indices{1, 3, 5, 7, 9, 11, 2, 4};

  BitVector bv = storage
                     .IndexSearch(FilterOp::kNe, SqlValue::Long(3),
                                  indices.data(), 8, false)
                     .TakeIfBitVector();

  ASSERT_EQ(bv.CountSetBits(), 5u);
}

TEST(SetIdStorageUnittest, IndexSearchLe) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);

  // {0, 3, 3, 6, 9, 9, 0, 3}
  std::vector<uint32_t> indices{1, 3, 5, 7, 9, 11, 2, 4};

  BitVector bv = storage
                     .IndexSearch(FilterOp::kLe, SqlValue::Long(3),
                                  indices.data(), 8, false)
                     .TakeIfBitVector();

  ASSERT_EQ(bv.CountSetBits(), 5u);
}

TEST(SetIdStorageUnittest, IndexSearchLt) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);

  // {0, 3, 3, 6, 9, 9, 0, 3}
  std::vector<uint32_t> indices{1, 3, 5, 7, 9, 11, 2, 4};

  BitVector bv = storage
                     .IndexSearch(FilterOp::kLt, SqlValue::Long(3),
                                  indices.data(), 8, false)
                     .TakeIfBitVector();

  ASSERT_EQ(bv.CountSetBits(), 2u);
}

TEST(SetIdStorageUnittest, IndexSearchGe) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);

  // {0, 3, 3, 6, 9, 9, 0, 3}
  std::vector<uint32_t> indices{1, 3, 5, 7, 9, 11, 2, 4};

  BitVector bv = storage
                     .IndexSearch(FilterOp::kGe, SqlValue::Long(6),
                                  indices.data(), 8, false)
                     .TakeIfBitVector();

  ASSERT_EQ(bv.CountSetBits(), 3u);
}

TEST(SetIdStorageUnittest, IndexSearchGt) {
  std::vector<uint32_t> storage_data{0, 0, 0, 3, 3, 3, 6, 6, 6, 9, 9, 9};
  SetIdStorage storage(&storage_data);

  // {0, 3, 3, 6, 9, 9, 0, 3}
  std::vector<uint32_t> indices{1, 3, 5, 7, 9, 11, 2, 4};

  BitVector bv = storage
                     .IndexSearch(FilterOp::kGt, SqlValue::Long(6),
                                  indices.data(), 8, false)
                     .TakeIfBitVector();

  ASSERT_EQ(bv.CountSetBits(), 2u);
}

}  // namespace
}  // namespace storage
}  // namespace trace_processor
}  // namespace perfetto
