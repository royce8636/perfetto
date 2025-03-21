/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "src/trace_processor/db/column/range_overlay.h"

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "perfetto/trace_processor/basic_types.h"
#include "src/trace_processor/containers/bit_vector.h"
#include "src/trace_processor/db/column/data_layer.h"
#include "src/trace_processor/db/column/fake_storage.h"
#include "src/trace_processor/db/column/numeric_storage.h"
#include "src/trace_processor/db/column/types.h"
#include "src/trace_processor/db/column/utils.h"
#include "test/gtest_and_gmock.h"

namespace perfetto::trace_processor::column {
namespace {

using testing::ElementsAre;
using testing::IsEmpty;
using Range = Range;

TEST(SelectorOverlay, SingleSearch) {
  Range range(3, 8);
  RangeOverlay storage(&range);
  auto fake = FakeStorageChain::SearchSubset(
      8, BitVector{false, false, false, true, false, false, false, false});
  auto chain = storage.MakeChain(std::move(fake));

  ASSERT_EQ(chain->SingleSearch(FilterOp::kEq, SqlValue::Long(0u), 0),
            SingleSearchResult::kMatch);
  ASSERT_EQ(chain->SingleSearch(FilterOp::kEq, SqlValue::Long(0u), 1),
            SingleSearchResult::kNoMatch);
}

TEST(SelectorOverlay, UniqueSearch) {
  Range range(1, 3);
  RangeOverlay storage(&range);
  auto fake = FakeStorageChain::SearchSubset(5, Range(2, 3));
  auto chain = storage.MakeChain(std::move(fake));

  uint32_t row = std::numeric_limits<uint32_t>::max();
  ASSERT_EQ(chain->UniqueSearch(FilterOp::kIsNotNull, SqlValue(), &row),
            UniqueSearchResult::kMatch);
  ASSERT_EQ(row, 1u);
}

TEST(SelectorOverlay, UniqueSearchLowOutOfBounds) {
  Range range(3, 8);
  RangeOverlay storage(&range);
  auto fake = FakeStorageChain::SearchSubset(8, Range(1, 2));
  auto chain = storage.MakeChain(std::move(fake));

  uint32_t row = std::numeric_limits<uint32_t>::max();
  ASSERT_EQ(chain->UniqueSearch(FilterOp::kIsNotNull, SqlValue(), &row),
            UniqueSearchResult::kNoMatch);
}

TEST(SelectorOverlay, UniqueSearchHighOutOfBounds) {
  Range range(3, 8);
  RangeOverlay storage(&range);
  auto fake = FakeStorageChain::SearchSubset(9, Range(8, 9));
  auto chain = storage.MakeChain(std::move(fake));

  uint32_t row = std::numeric_limits<uint32_t>::max();
  ASSERT_EQ(chain->UniqueSearch(FilterOp::kIsNotNull, SqlValue(), &row),
            UniqueSearchResult::kNoMatch);
}

TEST(RangeOverlay, SearchAll) {
  Range range(3, 8);
  RangeOverlay storage(&range);
  auto fake = FakeStorageChain::SearchAll(10);
  auto chain = storage.MakeChain(std::move(fake));

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(1, 4));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u, 2u, 3u));
}

TEST(RangeOverlay, SearchNone) {
  Range range(3, 8);
  RangeOverlay storage(&range);
  auto fake = FakeStorageChain::SearchNone(10);
  auto chain = storage.MakeChain(std::move(fake));

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(1, 4));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), IsEmpty());
}

TEST(RangeOverlay, SearchLimited) {
  auto fake = FakeStorageChain::SearchSubset(10, std::vector<uint32_t>{4});
  Range range(3, 5);
  RangeOverlay storage(&range);
  auto chain = storage.MakeChain(std::move(fake));

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(0, 2));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u));
}

TEST(RangeOverlay, SearchBitVector) {
  auto fake =
      FakeStorageChain::SearchSubset(8, BitVector({0, 1, 0, 1, 0, 1, 0, 0}));
  Range range(3, 6);
  RangeOverlay storage(&range);
  auto chain = storage.MakeChain(std::move(fake));

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(0, 3));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(0, 2));
}

TEST(RangeOverlay, IndexSearch) {
  auto fake =
      FakeStorageChain::SearchSubset(8, BitVector({0, 1, 0, 1, 0, 1, 0, 0}));
  Range range(3, 5);
  RangeOverlay storage(&range);
  auto chain = storage.MakeChain(std::move(fake));

  std::vector<uint32_t> table_idx{1u, 0u, 3u};
  RangeOrBitVector res = chain->IndexSearch(
      FilterOp::kGe, SqlValue::Long(0u),
      Indices{table_idx.data(), static_cast<uint32_t>(table_idx.size()),
              Indices::State::kNonmonotonic});
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u));
}

TEST(RangeOverlay, StableSort) {
  std::vector<uint32_t> numeric_data{100, 99, 2, 0, 1};
  NumericStorage<uint32_t> numeric(&numeric_data, ColumnType::kUint32, false);

  Range range(2, 4);
  RangeOverlay storage(&range);
  auto chain = storage.MakeChain(numeric.MakeChain());

  std::vector tokens{
      column::DataLayerChain::SortToken{0, 0},
      column::DataLayerChain::SortToken{1, 1},
      column::DataLayerChain::SortToken{2, 2},
  };
  chain->StableSort(tokens.data(), tokens.data() + tokens.size(),
                    column::DataLayerChain::SortDirection::kAscending);
  ASSERT_THAT(utils::ExtractPayloadForTesting(tokens), ElementsAre(1, 2, 0));
}

}  // namespace
}  // namespace perfetto::trace_processor::column
