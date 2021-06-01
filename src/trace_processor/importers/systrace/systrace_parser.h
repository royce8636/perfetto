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

#ifndef SRC_TRACE_PROCESSOR_IMPORTERS_SYSTRACE_SYSTRACE_PARSER_H_
#define SRC_TRACE_PROCESSOR_IMPORTERS_SYSTRACE_SYSTRACE_PARSER_H_

#include <ostream>

#include "src/trace_processor/types/trace_processor_context.h"

#include "perfetto/base/logging.h"
#include "perfetto/ext/base/string_utils.h"
#include "perfetto/ext/base/string_view.h"
#include "src/trace_processor/storage/trace_storage.h"

namespace perfetto {
namespace trace_processor {

namespace systrace_utils {

// Visible for unittesting.
enum class SystraceParseResult { kFailure = 0, kUnsupported, kSuccess };

// Visible for unittesting.
struct SystraceTracePoint {
  SystraceTracePoint() {}

  static SystraceTracePoint B(uint32_t tgid, base::StringView name) {
    return SystraceTracePoint('B', tgid, std::move(name), 0);
  }

  static SystraceTracePoint E(uint32_t tgid) {
    return SystraceTracePoint('E', tgid, {}, 0);
  }

  static SystraceTracePoint C(uint32_t tgid,
                              base::StringView name,
                              int64_t value) {
    return SystraceTracePoint('C', tgid, std::move(name), value);
  }

  static SystraceTracePoint S(uint32_t tgid,
                              base::StringView name,
                              int64_t cookie) {
    return SystraceTracePoint('S', tgid, std::move(name), cookie);
  }

  static SystraceTracePoint F(uint32_t tgid,
                              base::StringView name,
                              int64_t cookie) {
    return SystraceTracePoint('F', tgid, std::move(name), cookie);
  }

  SystraceTracePoint(char p, uint32_t tg, base::StringView n, int64_t v)
      : phase(p), tgid(tg), name(std::move(n)), value(v) {}

  // Phase can be one of B, E, C, S, F.
  char phase = '\0';

  uint32_t tgid = 0;

  // For phase = 'B' and phase = 'C' only.
  base::StringView name;

  // For phase = 'C' (counter value) and 'B', 'F' (async cookie).
  int64_t value = 0;

  // Visible for unittesting.
  friend std::ostream& operator<<(std::ostream& os,
                                  const SystraceTracePoint& point) {
    return os << "SystraceTracePoint{'" << point.phase << "', " << point.tgid
              << ", \"" << point.name.ToStdString() << "\", " << point.value
              << "}";
  }
};

// We have to handle trace_marker events of a few different types:
// 1. some random text
// 2. B|1636|pokeUserActivity
// 3. E|1636
// 4. C|1636|wq:monitor|0
// 5. S|1636|frame_capture|123
// 6. F|1636|frame_capture|456
// Counters emitted by chromium can have a further "category group" appended
// ("Blob" in the example below). We ignore the category group.
// 7. C|3209|TransfersBytesPendingOnDisk-value|0|Blob
inline SystraceParseResult ParseSystraceTracePoint(
    base::StringView str_untrimmed,
    SystraceTracePoint* out) {
  *out = {};

  // Strip trailing \n and \0. StringViews are not null-terminated, but the
  // writer could have appended a stray \0 depending on where the trace comes
  // from.
  size_t len = str_untrimmed.size();
  for (; len > 0; --len) {
    char last_char = str_untrimmed.at(len - 1);
    if (last_char != '\n' && last_char != '\0')
      break;
  }
  base::StringView str = str_untrimmed.substr(0, len);

  size_t off = 0;

  // This function reads the next field up to the next '|', '\0' or end(). It
  // advances |off| as it goes through fields.
  auto read_next_field = [&off, &str, len]() {
    for (size_t field_start = off;; ++off) {
      char c = off >= len ? '\0' : str.at(off);
      if (c == '|' || c == '\0') {
        auto res = str.substr(field_start, off - field_start);
        ++off;  // Eat the separator.
        return res;
      }
    }
  };

  auto f0_phase = read_next_field();
  if (PERFETTO_UNLIKELY(f0_phase.empty()))
    return SystraceParseResult::kFailure;
  out->phase = f0_phase.at(0);

  auto f1_tgid = read_next_field();
  auto opt_tgid = base::StringToUInt32(f1_tgid.ToStdString());
  out->tgid = opt_tgid.value_or(0);
  const bool has_tgid = opt_tgid.has_value();

  switch (out->phase) {
    case 'B': {  // Begin thread-scoped synchronous slice.
      if (!has_tgid)
        return SystraceParseResult::kFailure;
      auto f2_name = str.substr(off);  // It's fine even if |off| >= end().
      if (f2_name.empty()) {
        out->name = base::StringView("[empty slice name]");
      } else {
        out->name = f2_name;
      }
      return SystraceParseResult::kSuccess;
    }
    case 'E':  // End thread-scoped synchronous slice.
      // Some non-Android traces (Flutter) use just "E" (aosp/1244409). Allow
      // empty TGID on end slices. By design they are thread-scoped anyways.
      return SystraceParseResult::kSuccess;
    case 'S':    // Begin of async slice.
    case 'F': {  // End of async slice.
      auto f2_name = read_next_field();
      auto f3_cookie = read_next_field();
      auto maybe_cookie = base::StringToInt64(f3_cookie.ToStdString());
      if (PERFETTO_UNLIKELY(!has_tgid || f2_name.empty() || f3_cookie.empty() ||
                            !maybe_cookie)) {
        return SystraceParseResult::kFailure;
      }
      out->name = f2_name;
      out->value = *maybe_cookie;
      return SystraceParseResult::kSuccess;
    }
    case 'C': {  // Counter.
      auto f2_name = read_next_field();
      auto f3_value = read_next_field();
      auto maybe_value = base::StringToInt64(f3_value.ToStdString());
      if (PERFETTO_UNLIKELY(!has_tgid || f2_name.empty() || f3_value.empty() ||
                            !maybe_value)) {
        return SystraceParseResult::kFailure;
      }
      out->name = f2_name;
      out->value = *maybe_value;
      return SystraceParseResult::kSuccess;
    }
    default:
      if (str.find("trace_event_clock_sync:") == 0)
        return SystraceParseResult::kUnsupported;
      return SystraceParseResult::kFailure;
  }
}

// Visible for unittesting.
inline bool operator==(const SystraceTracePoint& x,
                       const SystraceTracePoint& y) {
  return std::tie(x.phase, x.tgid, x.name, x.value) ==
         std::tie(y.phase, y.tgid, y.name, y.value);
}

}  // namespace systrace_utils

class SystraceParser : public Destructible {
 public:
  static SystraceParser* GetOrCreate(TraceProcessorContext* context) {
    if (!context->systrace_parser) {
      context->systrace_parser.reset(new SystraceParser(context));
    }
    return static_cast<SystraceParser*>(context->systrace_parser.get());
  }
  ~SystraceParser() override;

  void ParsePrintEvent(int64_t ts, uint32_t pid, base::StringView event);

  void ParseTracingMarkWrite(int64_t ts,
                             uint32_t pid,
                             char trace_type,
                             bool trace_begin,
                             base::StringView trace_name,
                             uint32_t tgid,
                             int64_t value);

  void ParseZeroEvent(int64_t ts,
                      uint32_t pid,
                      int32_t flag,
                      base::StringView name,
                      uint32_t tgid,
                      int64_t value);

 private:
  explicit SystraceParser(TraceProcessorContext*);
  void ParseSystracePoint(int64_t ts,
                          uint32_t pid,
                          systrace_utils::SystraceTracePoint event);

  TraceProcessorContext* const context_;
  const StringId lmk_id_;
  const StringId screen_state_id_;
  const StringId cookie_id_;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_IMPORTERS_SYSTRACE_SYSTRACE_PARSER_H_
