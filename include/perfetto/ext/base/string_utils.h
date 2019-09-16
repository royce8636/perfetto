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

#ifndef INCLUDE_PERFETTO_EXT_BASE_STRING_UTILS_H_
#define INCLUDE_PERFETTO_EXT_BASE_STRING_UTILS_H_

#include <string>
#include <vector>

namespace perfetto {
namespace base {

inline char Lowercase(char c) {
  return ('A' <= c && c <= 'Z') ? (c -= 'A' - 'a') : c;
}

inline char Uppercase(char c) {
  return ('a' <= c && c <= 'z') ? (c += 'A' - 'a') : c;
}

bool StartsWith(const std::string& str, const std::string& prefix);
bool EndsWith(const std::string& str, const std::string& suffix);
bool Contains(const std::string& haystack, const std::string& needle);
bool CaseInsensitiveEqual(const std::string& first, const std::string& second);
std::string Join(const std::vector<std::string>& parts,
                 const std::string& delim);
std::vector<std::string> SplitString(const std::string& text,
                                     const std::string& delimiter);
std::string StripPrefix(const std::string& str, const std::string& prefix);
std::string StripSuffix(const std::string& str, const std::string& suffix);
std::string ToUpper(const std::string& str);
std::string StripChars(const std::string& str,
                       const std::string& chars,
                       char replacement);

}  // namespace base
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_EXT_BASE_STRING_UTILS_H_
