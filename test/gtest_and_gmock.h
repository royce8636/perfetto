/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef TEST_GTEST_AND_GMOCK_H_
#define TEST_GTEST_AND_GMOCK_H_

// This file is used to proxy the include of gtest/gmock headers and suppress
// the warnings generated by that.
// There are two ways we suppress gtest/gmock warnings:
// 1: Using config("test_warning_suppressions") in BUILD.gn
// 2: Via this file.
// 1 applies recursively also to the test translation units, 2 applies only
// to gmock/gtest includes.

#include "perfetto/base/build_config.h"

#if defined(__GNUC__)  // GCC & clang
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif  // __GNUC__

#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wshift-sign-overflow"

#if !PERFETTO_BUILDFLAG(PERFETTO_OS_NACL)
// -Wcomma isn't supported on NaCL.
#pragma GCC diagnostic ignored "-Wcomma"
#endif  // PERFETTO_OS_NACL

#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif  // TEST_GTEST_AND_GMOCK_H_
