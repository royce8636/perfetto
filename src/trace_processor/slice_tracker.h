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

#ifndef SRC_TRACE_PROCESSOR_SLICE_TRACKER_H_
#define SRC_TRACE_PROCESSOR_SLICE_TRACKER_H_

#include <stdint.h>

#include "src/trace_processor/trace_storage.h"

namespace perfetto {
namespace trace_processor {

class ArgsTracker;
class TraceProcessorContext;

class SliceTracker {
 public:
  using SetArgsCallback = std::function<void(ArgsTracker*, RowId row_id)>;

  explicit SliceTracker(TraceProcessorContext*);
  virtual ~SliceTracker();

  void BeginAndroid(int64_t timestamp,
                    uint32_t ftrace_tid,
                    uint32_t atrace_tgid,
                    StringId cat,
                    StringId name);

  // virtual for testing
  virtual void Begin(int64_t timestamp,
                     int64_t ref,
                     RefType ref_type,
                     StringId cat,
                     StringId name,
                     SetArgsCallback args_callback = SetArgsCallback());

  // virtual for testing
  virtual void Scoped(int64_t timestamp,
                      int64_t ref,
                      RefType ref_type,
                      StringId cat,
                      StringId name,
                      int64_t duration,
                      SetArgsCallback args_callback = SetArgsCallback());

  void EndAndroid(int64_t timestamp, uint32_t ftrace_tid, uint32_t atrace_tgid);

  // virtual for testing
  virtual void End(int64_t timestamp,
                   int64_t ref,
                   RefType ref_type,
                   StringId opt_cat = {},
                   StringId opt_name = {},
                   SetArgsCallback args_callback = SetArgsCallback());

 private:
  using SlicesStack = std::vector<std::pair<uint32_t /* row */, ArgsTracker>>;

  using StackMapKey = std::pair<int64_t, RefType>;
  struct StackMapHash {
    size_t operator()(const StackMapKey& p) const {
      base::Hash hash;
      hash.Update(p.first);
      hash.Update(p.second);
      return static_cast<size_t>(hash.digest());
    }
  };
  using StackMap = std::unordered_map<StackMapKey, SlicesStack, StackMapHash>;

  void StartSlice(int64_t timestamp,
                  int64_t duration,
                  int64_t ref,
                  RefType ref_type,
                  StringId cat,
                  StringId name,
                  SetArgsCallback args_callback);
  void CompleteSlice(StackMapKey ref_and_type);

  void MaybeCloseStack(int64_t end_ts, SlicesStack*);
  int64_t GetStackHash(const SlicesStack&);

  // Timestamp of the previous event. Used to discard events arriving out
  // of order.
  int64_t prev_timestamp_ = 0;

  TraceProcessorContext* const context_;
  StackMap stacks_;
  std::unordered_map<uint32_t, uint32_t> ftrace_to_atrace_tgid_;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_SLICE_TRACKER_H_
