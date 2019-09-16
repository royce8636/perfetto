/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "perfetto/ext/base/string_utils.h"
#include "test/gtest_and_gmock.h"

namespace perfetto {
namespace base {
namespace {

using testing::ElementsAre;

TEST(StringUtilsTest, StartsWith) {
  EXPECT_TRUE(StartsWith("", ""));
  EXPECT_TRUE(StartsWith("abc", ""));
  EXPECT_TRUE(StartsWith("abc", "a"));
  EXPECT_TRUE(StartsWith("abc", "ab"));
  EXPECT_TRUE(StartsWith("abc", "abc"));
  EXPECT_FALSE(StartsWith("abc", "abcd"));
  EXPECT_FALSE(StartsWith("aa", "ab"));
  EXPECT_FALSE(StartsWith("", "ab"));
}

TEST(StringUtilsTest, EndsWith) {
  EXPECT_TRUE(EndsWith("", ""));
  EXPECT_TRUE(EndsWith("abc", ""));
  EXPECT_TRUE(EndsWith("abc", "c"));
  EXPECT_TRUE(EndsWith("abc", "bc"));
  EXPECT_TRUE(EndsWith("abc", "abc"));
  EXPECT_FALSE(EndsWith("bcd", "abcd"));
  EXPECT_FALSE(EndsWith("abc", "abd"));
  EXPECT_FALSE(EndsWith("", "c"));
}

TEST(StringUtilsTest, ToHex) {
  EXPECT_EQ(ToHex(""), "");
  EXPECT_EQ(ToHex("abc123"), "616263313233");
}

TEST(StringUtilsTest, CaseInsensitiveEqual) {
  EXPECT_TRUE(CaseInsensitiveEqual("", ""));
  EXPECT_TRUE(CaseInsensitiveEqual("abc", "abc"));
  EXPECT_TRUE(CaseInsensitiveEqual("ABC", "abc"));
  EXPECT_TRUE(CaseInsensitiveEqual("abc", "ABC"));
  EXPECT_FALSE(CaseInsensitiveEqual("abc", "AB"));
  EXPECT_FALSE(CaseInsensitiveEqual("ab", "ABC"));
}

TEST(StringUtilsTest, SplitString) {
  EXPECT_THAT(SplitString("", ":"), ElementsAre());
  EXPECT_THAT(SplitString("a:b:c", ":"), ElementsAre("a", "b", "c"));
  EXPECT_THAT(SplitString("a::b::c", "::"), ElementsAre("a", "b", "c"));
  EXPECT_THAT(SplitString("::::a::b::::c::", "::"), ElementsAre("a", "b", "c"));
  EXPECT_THAT(SplitString("abc", ":"), ElementsAre("abc"));
  EXPECT_THAT(SplitString("abc", "::"), ElementsAre("abc"));
  EXPECT_THAT(SplitString("abc", ":"), ElementsAre("abc"));
  EXPECT_THAT(SplitString("abc", "::"), ElementsAre("abc"));
}

TEST(StringUtilsTest, Strip) {
  EXPECT_EQ(StripPrefix("abc", ""), "abc");
  EXPECT_EQ(StripPrefix("abc", "a"), "bc");
  EXPECT_EQ(StripPrefix("abc", "ab"), "c");
  EXPECT_EQ(StripPrefix("abc", "abc"), "");
  EXPECT_EQ(StripPrefix("abc", "abcd"), "abc");

  EXPECT_EQ(StripSuffix("abc", ""), "abc");
  EXPECT_EQ(StripSuffix("abc", "c"), "ab");
  EXPECT_EQ(StripSuffix("abc", "bc"), "a");
  EXPECT_EQ(StripSuffix("abc", "abc"), "");
  EXPECT_EQ(StripSuffix("abc", "ebcd"), "abc");

  EXPECT_EQ(StripChars("foobar", "", '_'), "foobar");
  EXPECT_EQ(StripChars("foobar", "x", '_'), "foobar");
  EXPECT_EQ(StripChars("foobar", "f", '_'), "_oobar");
  EXPECT_EQ(StripChars("foobar", "o", '_'), "f__bar");
  EXPECT_EQ(StripChars("foobar", "oa", '_'), "f__b_r");
  EXPECT_EQ(StripChars("foobar", "fbr", '_'), "_oo_a_");
  EXPECT_EQ(StripChars("foobar", "froab", '_'), "______");
}

}  // namespace
}  // namespace base
}  // namespace perfetto
