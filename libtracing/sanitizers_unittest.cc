/*
 * Copyright (C) 2017 The Android Open Source Project
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

// Smoke tests to verify that clang sanitizers are actually working.

#include <pthread.h>
#include <stdint.h>

#include <memory>

#include "gtest/gtest.h"

namespace perfetto {
namespace {

#if defined(ADDRESS_SANITIZER)
TEST(SanitizerTests, ASAN_UserAfterFree) {
  void* alloc = malloc(16);
  volatile char* mem = reinterpret_cast<volatile char*>(alloc);
  mem[0] = 1;
  mem[15] = 1;
  free(alloc);
  mem[0] = 2;
}
#endif  // ADDRESS_SANITIZER

#if defined(THREAD_SANITIZER)
TEST(SanitizerTests, TSAN_ThreadDataRace) {
  pthread_t thread;
  const int kNumRuns = 1000;
  volatile int race_var = 0;
  auto thread_main = [](void* race_var_ptr) -> void* {
    volatile int* my_race_var = reinterpret_cast<volatile int*>(race_var_ptr);
    printf("in thread %p", race_var_ptr);
    for (int i = 0; i < kNumRuns; i++)
      (*my_race_var)++;
    return nullptr;
  };
  void* arg = const_cast<void*>(reinterpret_cast<volatile void*>(&race_var));
  ASSERT_EQ(0, pthread_create(&thread, nullptr, thread_main, arg));
  for (int i = 0; i < kNumRuns; i++)
    race_var--;
  ASSERT_EQ(0, pthread_join(thread, nullptr));
}
#endif  // THREAD_SANITIZER

#if defined(MEMORY_SANITIZER)
TEST(SanitizerTests, MSAN_UninitializedMemory) {
  std::unique_ptr<int> mem(new int[10]);
  volatile int* x = reinterpret_cast<volatile int*>(mem.get());
  if(x[rand() % 10] == 42)
    printf("\n");
}
#endif

#if defined(LEAK_SANITIZER)
TEST(SanitizerTests, LSAN_LeakMalloc) {
  void* alloc = malloc(16);
  reinterpret_cast<volatile char*>(alloc)[0] = 1;
  alloc = malloc(16);
  reinterpret_cast<volatile char*>(alloc)[0] = 2;
  free(alloc);
}

TEST(SanitizerTests, LSAN_LeakCppNew) {
  std::unique_ptr<int> alloc(new int(1));
  *reinterpret_cast<volatile char*>(alloc.get()) = 1;
  alloc.release();
  alloc.reset(new int(2));
  *reinterpret_cast<volatile char*>(alloc.get()) = 2;
}
#endif  // LEAK_SANITIZER

#if defined(UNDEFINED_SANITIZER)
TEST(SanitizerTests, UBSAN_DivisionByZero) {
  volatile float div = 1;
  float res = 3 / (div - 1);
  ASSERT_GT(res, -1.0f);  // just use |res| to make the compiler happy.
}

TEST(SanitizerTests, UBSAN_ShiftExponent) {
  volatile uint32_t n = 32;
  volatile uint32_t shift = 31;
  uint64_t res = n << (shift + 3);
  ASSERT_NE(1u, res);  // just use |res| to make the compiler happy.
}
#endif  // UNDEFINED_SANITIZER

}  // namespace
}  // namespace perfetto
