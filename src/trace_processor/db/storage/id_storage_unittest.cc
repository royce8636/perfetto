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
#include "src/trace_processor/db/storage/id_storage.h"
#include <limits>

#include "src/trace_processor/db/storage/types.h"
#include "test/gtest_and_gmock.h"

namespace perfetto {
namespace trace_processor {

inline bool operator==(const RowMap::Range& a, const RowMap::Range& b) {
  return std::tie(a.start, a.end) == std::tie(b.start, b.end);
}

namespace storage {
namespace {

using Range = RowMap::Range;

TEST(IdStorageUnittest, InvalidSearchConstraints) {
  IdStorage storage(100);
  Range test_range(10, 20);
  Range empty_range;

  // NULL checks
  SqlValue val;
  val.type = SqlValue::kNull;
  Range search_result =
      storage.Search(FilterOp::kIsNull, val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, empty_range);
  search_result =
      storage.Search(FilterOp::kIsNotNull, val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, test_range);

  // FilterOp checks
  search_result =
      storage.Search(FilterOp::kGlob, SqlValue::Long(15), test_range)
          .TakeIfRange();
  ASSERT_EQ(search_result, empty_range);
  search_result =
      storage.Search(FilterOp::kRegex, SqlValue::Long(15), test_range)
          .TakeIfRange();
  ASSERT_EQ(search_result, empty_range);

  // Type checks
  search_result =
      storage.Search(FilterOp::kGe, SqlValue::String("cheese"), test_range)
          .TakeIfRange();
  ASSERT_EQ(search_result, empty_range);

  // Value bounds
  SqlValue max_val = SqlValue::Long(
      static_cast<int64_t>(std::numeric_limits<uint32_t>::max()) + 10);
  search_result =
      storage.Search(FilterOp::kGe, max_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, empty_range);
  search_result =
      storage.Search(FilterOp::kGt, max_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, empty_range);
  search_result =
      storage.Search(FilterOp::kEq, max_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, empty_range);

  search_result =
      storage.Search(FilterOp::kLe, max_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, test_range);
  search_result =
      storage.Search(FilterOp::kLt, max_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, test_range);
  search_result =
      storage.Search(FilterOp::kNe, max_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, test_range);

  SqlValue min_val = SqlValue::Long(
      static_cast<int64_t>(std::numeric_limits<uint32_t>::min()) - 1);
  search_result =
      storage.Search(FilterOp::kGe, min_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, test_range);
  search_result =
      storage.Search(FilterOp::kGt, min_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, test_range);
  search_result =
      storage.Search(FilterOp::kNe, min_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, test_range);

  search_result =
      storage.Search(FilterOp::kLe, min_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, empty_range);
  search_result =
      storage.Search(FilterOp::kLt, min_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, empty_range);
  search_result =
      storage.Search(FilterOp::kEq, min_val, test_range).TakeIfRange();
  ASSERT_EQ(search_result, empty_range);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicEqSimple) {
  IdStorage storage(100);
  Range range = storage.Search(FilterOp::kEq, SqlValue::Long(15), Range(10, 20))
                    .TakeIfRange();
  ASSERT_EQ(range.size(), 1u);
  ASSERT_EQ(range.start, 15u);
  ASSERT_EQ(range.end, 16u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicEqOnRangeBoundary) {
  IdStorage storage(100);
  Range range = storage.Search(FilterOp::kEq, SqlValue::Long(20), Range(10, 20))
                    .TakeIfRange();
  ASSERT_EQ(range.size(), 0u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicEqOutsideRange) {
  IdStorage storage(100);
  Range range = storage.Search(FilterOp::kEq, SqlValue::Long(25), Range(10, 20))
                    .TakeIfRange();
  ASSERT_EQ(range.size(), 0u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicEqTooBig) {
  IdStorage storage(100);
  Range range =
      storage.Search(FilterOp::kEq, SqlValue::Long(125), Range(10, 20))
          .TakeIfRange();
  ASSERT_EQ(range.size(), 0u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicLe) {
  IdStorage storage(100);
  Range range = storage.Search(FilterOp::kLe, SqlValue::Long(50), Range(30, 70))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 30u);
  ASSERT_EQ(range.end, 51u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicLt) {
  IdStorage storage(100);
  Range range = storage.Search(FilterOp::kLt, SqlValue::Long(50), Range(30, 70))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 30u);
  ASSERT_EQ(range.end, 50u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicGe) {
  IdStorage storage(100);
  Range range = storage.Search(FilterOp::kGe, SqlValue::Long(40), Range(30, 70))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 40u);
  ASSERT_EQ(range.end, 70u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicGt) {
  IdStorage storage(100);
  Range range = storage.Search(FilterOp::kGt, SqlValue::Long(40), Range(30, 70))
                    .TakeIfRange();
  ASSERT_EQ(range.start, 41u);
  ASSERT_EQ(range.end, 70u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicNe) {
  IdStorage storage(100);
  BitVector bv =
      storage.Search(FilterOp::kNe, SqlValue::Long(40), Range(30, 70))
          .TakeIfBitVector();
  ASSERT_EQ(bv.CountSetBits(), 39u);
}

TEST(IdStorageUnittest, BinarySearchIntrinsicNeInvalidNum) {
  IdStorage storage(100);
  Range r = storage.Search(FilterOp::kNe, SqlValue::Long(-1), Range(30, 70))
                .TakeIfRange();
  ASSERT_EQ(r.size(), 40u);
}

TEST(IdStorageUnittest, Sort) {
  std::vector<uint32_t> order{4, 3, 6, 1, 5};
  IdStorage storage(10);
  storage.Sort(order.data(), 5);

  std::vector<uint32_t> sorted_order{1, 3, 4, 5, 6};
  ASSERT_EQ(order, sorted_order);
}

}  // namespace
}  // namespace storage
}  // namespace trace_processor
}  // namespace perfetto
