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

#include "src/profiling/memory/sampler.h"

#include "gtest/gtest.h"

#include <thread>

#include "src/profiling/memory/client.h"  // For PThreadKey.

namespace perfetto {
namespace profiling {
namespace {

TEST(SamplerTest, TestLarge) {
  PThreadKey key(ThreadLocalSamplingData::KeyDestructor);
  ASSERT_TRUE(key.valid());
  EXPECT_EQ(SampleSize(key.get(), 1024, 512, malloc, free), 1024);
}

TEST(SamplerTest, TestSmall) {
  PThreadKey key(ThreadLocalSamplingData::KeyDestructor);
  ASSERT_TRUE(key.valid());
  EXPECT_EQ(SampleSize(key.get(), 511, 512, malloc, free), 512);
}

TEST(SamplerTest, TestSmallFromThread) {
  PThreadKey key(ThreadLocalSamplingData::KeyDestructor);
  ASSERT_TRUE(key.valid());
  std::thread th([&key] {
    EXPECT_EQ(SampleSize(key.get(), 511, 512, malloc, free), 512);
  });
  std::thread th2([&key] {
    // The threads should have separate state.
    EXPECT_EQ(SampleSize(key.get(), 511, 512, malloc, free), 512);
  });
  th.join();
  th2.join();
}

}  // namespace
}  // namespace profiling
}  // namespace perfetto
