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

#include "src/trace_processor/db/column/selector_overlay.h"

#include <cstdint>
#include <vector>

#include "perfetto/trace_processor/basic_types.h"
#include "src/trace_processor/containers/bit_vector.h"
#include "src/trace_processor/db/column/fake_storage.h"
#include "src/trace_processor/db/column/types.h"
#include "src/trace_processor/db/column/utils.h"
#include "test/gtest_and_gmock.h"

namespace perfetto::trace_processor::column {
namespace {

using testing::ElementsAre;
using testing::IsEmpty;

TEST(SelectorOverlay, SearchAll) {
  BitVector selector{0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1};
  auto fake = FakeStorage::SearchAll(10);
  SelectorOverlay storage(&selector);
  auto chain = storage.MakeChain(fake->MakeChain());

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(1, 4));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u, 2u, 3u));
}

TEST(SelectorOverlay, SearchNone) {
  BitVector selector{0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1};
  auto fake = FakeStorage::SearchNone(10);
  SelectorOverlay storage(&selector);
  auto chain = storage.MakeChain(fake->MakeChain());

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(1, 4));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), IsEmpty());
}

TEST(SelectorOverlay, SearchLimited) {
  BitVector selector{0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1};
  auto fake = FakeStorage::SearchSubset(10, Range(4, 5));
  SelectorOverlay storage(&selector);
  auto chain = storage.MakeChain(fake->MakeChain());

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(1, 5));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(2u));
}

TEST(SelectorOverlay, SearchBitVector) {
  BitVector selector{0, 1, 1, 0, 0, 1, 1, 0};
  auto fake = FakeStorage::SearchSubset(8, BitVector({0, 1, 0, 1, 0, 1, 0, 0}));
  SelectorOverlay storage(&selector);
  auto chain = storage.MakeChain(fake->MakeChain());

  auto res = chain->Search(FilterOp::kGe, SqlValue::Long(0u), Range(0, 4));
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(0, 2));
}

TEST(SelectorOverlay, IndexSearch) {
  BitVector selector{0, 1, 1, 0, 0, 1, 1, 0};
  auto fake = FakeStorage::SearchSubset(8, BitVector({0, 1, 0, 1, 0, 1, 0, 0}));
  SelectorOverlay storage(&selector);
  auto chain = storage.MakeChain(fake->MakeChain());

  std::vector<uint32_t> table_idx{1u, 0u, 3u};
  RangeOrBitVector res = chain->IndexSearch(
      FilterOp::kGe, SqlValue::Long(0u),
      Indices{table_idx.data(), static_cast<uint32_t>(table_idx.size()),
              Indices::State::kNonmonotonic});
  ASSERT_THAT(utils::ToIndexVectorForTests(res), ElementsAre(1u));
}

TEST(SelectorOverlay, OrderedIndexSearchTrivial) {
  BitVector selector{1, 0, 1, 0, 1};
  auto fake = FakeStorage::SearchAll(5);
  SelectorOverlay storage(&selector);
  auto chain = storage.MakeChain(fake->MakeChain());

  std::vector<uint32_t> table_idx{1u, 0u, 2u};
  Range res = chain->OrderedIndexSearch(
      FilterOp::kGe, SqlValue::Long(0u),
      Indices{table_idx.data(), static_cast<uint32_t>(table_idx.size()),
              Indices::State::kNonmonotonic});
  ASSERT_EQ(res.start, 0u);
  ASSERT_EQ(res.end, 3u);
}

TEST(SelectorOverlay, OrderedIndexSearchNone) {
  BitVector selector{1, 0, 1, 0, 1};
  auto fake = FakeStorage::SearchNone(5);
  SelectorOverlay storage(&selector);
  auto chain = storage.MakeChain(fake->MakeChain());

  std::vector<uint32_t> table_idx{1u, 0u, 2u};
  Range res = chain->OrderedIndexSearch(
      FilterOp::kGe, SqlValue::Long(0u),
      Indices{table_idx.data(), static_cast<uint32_t>(table_idx.size()),
              Indices::State::kNonmonotonic});
  ASSERT_EQ(res.size(), 0u);
}

}  // namespace
}  // namespace perfetto::trace_processor::column
