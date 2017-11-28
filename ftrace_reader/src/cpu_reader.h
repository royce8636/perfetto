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

#ifndef FTRACE_READER_CPU_READER_H_
#define FTRACE_READER_CPU_READER_H_

#include <stdint.h>

#include <array>
#include <memory>

#include "base/scoped_file.h"
#include "ftrace_reader/ftrace_controller.h"
#include "gtest/gtest_prod.h"
#include "proto_translation_table.h"

namespace perfetto {

class ProtoTranslationTable;

namespace protos {
namespace pbzero {
class FtraceEventBundle;
}  // namespace pbzero
}  // namespace protos

// Class for efficient 'is event with id x enabled?' tests.
// Mirrors the data in a FtraceConfig but in a format better suited
// to be consumed by CpuReader.
class EventFilter {
 public:
  EventFilter(const ProtoTranslationTable&, std::set<std::string>);
  ~EventFilter();

  bool IsEventEnabled(size_t ftrace_event_id) const {
    if (ftrace_event_id == 0 || ftrace_event_id > enabled_ids_.size()) {
      return false;
    }
    return enabled_ids_[ftrace_event_id];
  }

  const std::set<std::string>& enabled_names() const { return enabled_names_; }

 private:
  EventFilter(const EventFilter&) = delete;
  EventFilter& operator=(const EventFilter&) = delete;

  const std::vector<bool> enabled_ids_;
  std::set<std::string> enabled_names_;
};

class CpuReader {
 public:
  CpuReader(const ProtoTranslationTable*, size_t cpu, base::ScopedFile fd);
  ~CpuReader();

  bool Drain(
      const std::array<const EventFilter*, kMaxSinks>&,
      const std::array<
          protozero::ProtoZeroMessageHandle<protos::pbzero::FtraceEventBundle>,
          kMaxSinks>&);
  int GetFileDescriptor();

 private:
  FRIEND_TEST(CpuReaderTest, ReadAndAdvanceNumber);
  FRIEND_TEST(CpuReaderTest, ReadAndAdvancePlainStruct);
  FRIEND_TEST(CpuReaderTest, ReadAndAdvanceComplexStruct);
  FRIEND_TEST(CpuReaderTest, ReadAndAdvanceUnderruns);
  FRIEND_TEST(CpuReaderTest, ReadAndAdvanceAtEnd);
  FRIEND_TEST(CpuReaderTest, ReadAndAdvanceOverruns);
  FRIEND_TEST(CpuReaderTest, ParseSimpleEvent);

  template <typename T>
  static bool ReadAndAdvance(const uint8_t** ptr, const uint8_t* end, T* out) {
    if (*ptr > end - sizeof(T))
      return false;
    memcpy(out, *ptr, sizeof(T));
    *ptr += sizeof(T);
    return true;
  }

  static bool ParsePage(size_t cpu,
                        const uint8_t* ptr,
                        size_t ptr_size,
                        const EventFilter*,
                        protos::pbzero::FtraceEventBundle*,
                        const ProtoTranslationTable* table);
  uint8_t* GetBuffer();
  CpuReader(const CpuReader&) = delete;
  CpuReader& operator=(const CpuReader&) = delete;

  const ProtoTranslationTable* table_;
  const size_t cpu_;
  base::ScopedFile fd_;
  std::unique_ptr<uint8_t[]> buffer_;
};

}  // namespace perfetto

#endif  // FTRACE_READER_CPU_READER_H_
