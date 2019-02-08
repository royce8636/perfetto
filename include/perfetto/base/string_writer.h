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

#ifndef INCLUDE_PERFETTO_BASE_STRING_WRITER_H_
#define INCLUDE_PERFETTO_BASE_STRING_WRITER_H_

#include <math.h>
#include <limits>

#include "perfetto/base/logging.h"
#include "perfetto/base/string_view.h"

namespace perfetto {
namespace base {

// A helper class which writes formatted data to a string buffer.
// This is used in the trace processor where we write O(GBs) of strings and
// sprintf is too slow.
class StringWriter {
 public:
  // Creates a string buffer from a char buffer and length.
  StringWriter(char* buffer, size_t size) : buffer_(buffer), size_(size) {}

  // Appends a char to the buffer.
  void AppendChar(char in) {
    PERFETTO_DCHECK(pos_ + 1 <= size_);
    buffer_[pos_++] = in;
  }

  // Appends a length delimited string to the buffer.
  void AppendString(const char* in, size_t n) {
    PERFETTO_DCHECK(pos_ + n <= size_);
    memcpy(&buffer_[pos_], in, n);
    pos_ += n;
  }

  // Appends a null-terminated string literal to the buffer.
  template <size_t N>
  inline void AppendLiteral(const char (&in)[N]) {
    AppendString(in, N - 1);
  }

  // Appends a StringView to the buffer.
  void AppendString(StringView data) { AppendString(data.data(), data.size()); }

  // Appends an integer to the buffer.
  void AppendInt(int64_t value) { AppendPaddedInt<'0', 0>(value); }

  // Appends an integer to the buffer, padding with |padchar| if the number of
  // digits of the integer is less than |padding|.
  template <char padchar, uint64_t padding>
  void AppendPaddedInt(int64_t sign_value) {
    // Need to add 2 to the number of digits to account for minus sign and
    // rounding down of digits10.
    constexpr auto kMaxDigits = std::numeric_limits<uint64_t>::digits10 + 2;
    constexpr auto kSizeNeeded = kMaxDigits > padding ? kMaxDigits : padding;
    PERFETTO_DCHECK(pos_ + kSizeNeeded <= size_);

    char data[kSizeNeeded];
    const bool negate = signbit(sign_value);
    uint64_t value = static_cast<uint64_t>(abs(sign_value));

    size_t idx;
    for (idx = kSizeNeeded - 1; value >= 10;) {
      char digit = value % 10;
      value /= 10;
      data[idx--] = digit + '0';
    }
    data[idx--] = static_cast<char>(value) + '0';

    if (padding > 0) {
      size_t num_digits = kSizeNeeded - 1 - idx;
      for (size_t i = num_digits; i < padding; i++) {
        data[idx--] = padchar;
      }
    }

    if (negate)
      buffer_[pos_++] = '-';
    AppendString(&data[idx + 1], kSizeNeeded - idx - 1);
  }

  // Appends a double to the buffer.
  void AppendDouble(double value) {
    // TODO(lalitm): trying to optimize this is premature given we almost never
    // print doubles. Reevaluate this in the future if we do print them more.
    size_t res = static_cast<size_t>(
        snprintf(buffer_ + pos_, size_ - pos_, "%lf", value));
    PERFETTO_DCHECK(pos_ + res < size_);
    pos_ += res;
  }

  char* GetCString() {
    // TODO(lalitm): this may need to be changed in the future to return a
    // StringView if we find that we will want embedded nulls in our strings.
    PERFETTO_DCHECK(pos_ < size_);
    buffer_[pos_] = '\0';
    return buffer_;
  }

 private:
  char* buffer_ = nullptr;
  size_t size_ = 0;
  size_t pos_ = 0;
};

}  // namespace base
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_BASE_STRING_WRITER_H_
