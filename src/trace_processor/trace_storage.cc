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

#include "src/trace_processor/trace_storage.h"

#include <string.h>

namespace perfetto {
namespace trace_processor {

TraceStorage::~TraceStorage() {}

void TraceStorage::PushSchedSwitch(uint32_t cpu,
                                   uint64_t timestamp,
                                   uint32_t prev_pid,
                                   uint32_t prev_state,
                                   const char* prev_comm,
                                   size_t prev_comm_len,
                                   uint32_t next_pid) {
  SchedSwitchEvent* prev = &last_sched_per_cpu_[cpu];

  // If we had a valid previous event, then inform the storage about the
  // slice.
  if (prev->valid()) {
    uint64_t duration = timestamp - prev->timestamp;
    cpu_events_[cpu].AddSlice(prev->timestamp, duration, prev->prev_thread_id);
  }

  // If the this events previous pid does not match the previous event's next
  // pid, make a note of this.
  if (prev_pid != prev->next_pid) {
    stats_.mismatched_sched_switch_tids_++;
  }

  // Update the map with the current event.
  prev->cpu = cpu;
  prev->timestamp = timestamp;
  prev->prev_pid = prev_pid;
  prev->prev_state = prev_state;
  prev->prev_thread_id = InternString(prev_comm, prev_comm_len);
  prev->next_pid = next_pid;
}

TraceStorage::StringId TraceStorage::InternString(const char* data,
                                                  size_t length) {
  uint32_t hash = 0;
  for (uint64_t i = 0; i < length; ++i) {
    hash = static_cast<uint32_t>(data[i]) + (hash * 31);
  }
  auto id_it = string_index_.find(hash);
  if (id_it != string_index_.end()) {
    // TODO(lalitm): check if this DCHECK happens and if so, then change hash
    // to 64bit.
    PERFETTO_DCHECK(string_pool_[id_it->second] == std::string(data, length));
    return id_it->second;
  }
  string_pool_.emplace_back(data, length);
  StringId string_id = string_pool_.size() - 1;
  string_index_.emplace(hash, string_id);
  return string_id;
}

}  // namespace trace_processor
}  // namespace perfetto
