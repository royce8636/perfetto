/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "src/trace_processor/importers/common/args_translation_table.h"
#include "test/gtest_and_gmock.h"

namespace perfetto {
namespace trace_processor {
namespace {

TEST(ArgsTranslationTable, EmptyTableByDefault) {
  TraceStorage storage;
  ArgsTranslationTable table(&storage);
  EXPECT_EQ(table.TranslateChromeHistogramHashForTesting(1), base::nullopt);
}

TEST(ArgsTranslationTable, TranslatesHashes) {
  TraceStorage storage;
  ArgsTranslationTable table(&storage);
  table.AddChromeHistogramTranslationRule(1, "hash1");
  table.AddChromeHistogramTranslationRule(10, "hash2");
  EXPECT_EQ(table.TranslateChromeHistogramHashForTesting(1),
            base::Optional<base::StringView>("hash1"));
  EXPECT_EQ(table.TranslateChromeHistogramHashForTesting(10),
            base::Optional<base::StringView>("hash2"));
  EXPECT_EQ(table.TranslateChromeHistogramHashForTesting(2), base::nullopt);
}

}  // namespace
}  // namespace trace_processor
}  // namespace perfetto
