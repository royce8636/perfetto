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

#ifndef SRC_TRACE_PROCESSOR_TRACE_PARSER_H_
#define SRC_TRACE_PROCESSOR_TRACE_PARSER_H_

#include <memory>

namespace perfetto {
namespace trace_processor {

// Base interface for trace parsers (JsonTraceParser, ProtoTraceParser).
class TraceParser {
 public:
  virtual ~TraceParser();

  // Pushes more data into the trace parser. There is no requirement for the
  // caller to match line/protos boundaries. The parser class has to deal with
  // intermediate buffering lines/protos that span across different chunks.
  // The buffer size is guaranteed to be > 0.
  // Returns true if the data has been succesfully parsed, false if some
  // unrecoverable parsing error happened and no more chunks should be pushed.
  virtual bool Parse(std::unique_ptr<uint8_t[]>, size_t) = 0;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_TRACE_PARSER_H_
