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
#include <vector>

#include "perfetto/trace_processor/basic_types.h"
#include "src/trace_processor/db/column/fake_storage.h"
#include "src/trace_processor/db/column/types.h"
#include "src/trace_processor/db/column/utils.h"
#include "test/gtest_and_gmock.h"

namespace perfetto::trace_processor::column {
namespace {

using testing::ElementsAre;
using testing::IsEmpty;
using Range = Range;

TEST(RangeOverlay, SearchAll) {
  Range range(3, 8);
  RangeOverlay storage(&range);
  auto fake = FakeStorage::SearchAll(10);
  auto queryable = storage.MakeChain(fake->MakeChain());

  auto res = queryable->Search(FilterOp::kGe, SqlValue::Long(0u), Range(1, 4));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u, 2u, 3u));
}

TEST(RangeOverlay, SearchNone) {
  Range range(3, 8);
  RangeOverlay storage(&range);
  auto fake = FakeStorage::SearchNone(10);
  auto queryable = storage.MakeChain(fake->MakeChain());

  auto res = queryable->Search(FilterOp::kGe, SqlValue::Long(0u), Range(1, 4));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), IsEmpty());
}

TEST(RangeOverlay, SearchLimited) {
  auto fake = FakeStorage::SearchSubset(10, std::vector<uint32_t>{4});
  Range range(3, 5);
  RangeOverlay storage(&range);
  auto queryable = storage.MakeChain(fake->MakeChain());

  auto res = queryable->Search(FilterOp::kGe, SqlValue::Long(0u), Range(0, 2));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u));
}

TEST(RangeOverlay, SearchBitVector) {
  auto fake = FakeStorage::SearchSubset(8, BitVector({0, 1, 0, 1, 0, 1, 0, 0}));
  Range range(3, 6);
  RangeOverlay storage(&range);
  auto queryable = storage.MakeChain(fake->MakeChain());

  auto res = queryable->Search(FilterOp::kGe, SqlValue::Long(0u), Range(0, 3));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(0, 2));
}

TEST(RangeOverlay, IndexSearch) {
  auto fake = FakeStorage::SearchSubset(8, BitVector({0, 1, 0, 1, 0, 1, 0, 0}));
  Range range(3, 5);
  RangeOverlay storage(&range);
  auto queryable = storage.MakeChain(fake->MakeChain());

  std::vector<uint32_t> table_idx{1u, 0u, 3u};
  RangeOrBitVector res = queryable->IndexSearch(
      FilterOp::kGe, SqlValue::Long(0u),
      Indices{table_idx.data(), static_cast<uint32_t>(table_idx.size()),
              Indices::State::kNonmonotonic});
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u));
}

}  // namespace
}  // namespace perfetto::trace_processor::column
