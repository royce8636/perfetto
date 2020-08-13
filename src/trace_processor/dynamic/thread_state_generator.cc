/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "src/trace_processor/dynamic/thread_state_generator.h"

#include <memory>
#include <set>

#include "src/trace_processor/types/trace_processor_context.h"

namespace perfetto {
namespace trace_processor {

ThreadStateGenerator::ThreadStateGenerator(TraceProcessorContext* context)
    : running_string_id_(context->storage->InternString("Running")),
      runnable_string_id_(context->storage->InternString("R")),
      context_(context) {}

ThreadStateGenerator::~ThreadStateGenerator() = default;

util::Status ThreadStateGenerator::ValidateConstraints(
    const QueryConstraints&) {
  return util::OkStatus();
}

std::unique_ptr<Table> ThreadStateGenerator::ComputeTable(
    const std::vector<Constraint>&,
    const std::vector<Order>&) {
  if (!unsorted_thread_state_table_) {
    int64_t trace_end_ts =
        context_->storage->GetTraceTimestampBoundsNs().second;

    unsorted_thread_state_table_ = ComputeThreadStateTable(trace_end_ts);

    // We explicitly sort by ts here as ComputeThreadStateTable does not insert
    // rows in sorted order but we expect our clients to always want to sort
    // on ts. Writing ComputeThreadStateTable to insert in sorted order is
    // more trouble than its worth.
    sorted_thread_state_table_ = unsorted_thread_state_table_->Sort(
        {unsorted_thread_state_table_->ts().ascending()});
  }
  PERFETTO_CHECK(sorted_thread_state_table_);
  return std::unique_ptr<Table>(new Table(sorted_thread_state_table_->Copy()));
}

std::unique_ptr<tables::ThreadStateTable>
ThreadStateGenerator::ComputeThreadStateTable(int64_t trace_end_ts) {
  std::unique_ptr<tables::ThreadStateTable> table(new tables::ThreadStateTable(
      context_->storage->mutable_string_pool(), nullptr));

  const auto& raw_sched = context_->storage->sched_slice_table();
  const auto& instants = context_->storage->instant_table();

  // In both tables, exclude utid == 0 which represents the idle thread.
  auto sched = raw_sched.Filter({raw_sched.utid().ne(0)});
  auto waking = instants.Filter(
      {instants.name().eq("sched_waking"), instants.ref().ne(0)});

  // We prefer to use waking if at all possible and fall back to wakeup if not
  // available.
  if (waking.row_count() == 0) {
    waking = instants.Filter(
        {instants.name().eq("sched_wakeup"), instants.ref().ne(0)});
  }

  const auto& sched_ts = sched.GetTypedColumnByName<int64_t>("ts");
  const auto& waking_ts = waking.GetTypedColumnByName<int64_t>("ts");

  uint32_t sched_idx = 0;
  uint32_t waking_idx = 0;
  std::unordered_map<UniqueTid, uint32_t> state_map;
  while (sched_idx < sched.row_count() || waking_idx < waking.row_count()) {
    // We go through both tables, picking the earliest timestamp from either
    // to process that event.
    if (waking_idx >= waking.row_count() ||
        (sched_idx < sched.row_count() &&
         sched_ts[sched_idx] <= waking_ts[waking_idx])) {
      AddSchedEvent(sched, sched_idx++, state_map, trace_end_ts, table.get());
    } else {
      AddWakingEvent(waking, waking_idx++, state_map, table.get());
    }
  }
  return table;
}

void ThreadStateGenerator::UpdateDurIfNotRunning(
    int64_t new_ts,
    uint32_t row,
    tables::ThreadStateTable* table) {
  // The duration should always have been left dangling and the new timestamp
  // should happen after the older one (as we go through events in ts order).
  PERFETTO_DCHECK(table->dur()[row] == -1);
  PERFETTO_DCHECK(new_ts > table->ts()[row]);

  if (table->state()[row] == running_string_id_)
    return;
  table->mutable_dur()->Set(row, new_ts - table->ts()[row]);
}

void ThreadStateGenerator::AddSchedEvent(
    const Table& sched,
    uint32_t sched_idx,
    std::unordered_map<UniqueTid, uint32_t>& state_map,
    int64_t trace_end_ts,
    tables::ThreadStateTable* table) {
  UniqueTid utid = sched.GetTypedColumnByName<uint32_t>("utid")[sched_idx];
  int64_t ts = sched.GetTypedColumnByName<int64_t>("ts")[sched_idx];

  // Update the duration on the existing event.
  auto it = state_map.find(utid);
  if (it != state_map.end()) {
    // Don't update dur from -1 if we were already running (can happen because
    // of data loss).
    UpdateDurIfNotRunning(ts, it->second, table);
  }

  // Undo the expansion of the final sched slice for each CPU to the end of the
  // trace by setting the duration back to -1. This counteracts the code in
  // SchedEventTracker::FlushPendingEvents
  // TODO(lalitm): remove this hack when we stop expanding the last slice to the
  // end of the trace.
  int64_t dur = sched.GetTypedColumnByName<int64_t>("dur")[sched_idx];
  if (ts + dur == trace_end_ts) {
    dur = -1;
  }

  // First, we add the sched slice itself as "Running" with the other fields
  // unchanged.
  tables::ThreadStateTable::Row sched_row;
  sched_row.ts = ts;
  sched_row.dur = dur;
  sched_row.cpu = sched.GetTypedColumnByName<uint32_t>("cpu")[sched_idx];
  sched_row.state = running_string_id_;
  sched_row.utid = utid;
  state_map[utid] = table->Insert(sched_row).row;

  // If the sched row had a negative duration, don't add the dangling
  // descheduled slice as it would be meaningless.
  if (sched_row.dur == -1) {
    return;
  }

  // Next, we add a dangling, slice to represent the descheduled state with the
  // given end state from the sched event. The duration will be updated by the
  // next event (sched or waking) that we see.
  tables::ThreadStateTable::Row row;
  row.ts = sched_row.ts + sched_row.dur;
  row.dur = -1;
  row.state = sched.GetTypedColumnByName<StringId>("end_state")[sched_idx];
  row.utid = utid;
  state_map[utid] = table->Insert(row).row;
}

void ThreadStateGenerator::AddWakingEvent(
    const Table& waking,
    uint32_t waking_idx,
    std::unordered_map<UniqueTid, uint32_t>& state_map,
    tables::ThreadStateTable* table) {
  int64_t ts = waking.GetTypedColumnByName<int64_t>("ts")[waking_idx];
  UniqueTid utid = static_cast<UniqueTid>(
      waking.GetTypedColumnByName<int64_t>("ref")[waking_idx]);

  auto it = state_map.find(utid);
  if (it != state_map.end()) {
    // As counter-intuitive as it seems, occassionally we can get a waking
    // event for a thread which is currently running.
    //
    // There are two cases when this can happen:
    // 1. The kernel legitimately send a waking event for a "running" thread
    //    because the thread was woken up before the kernel switched away
    //    from it. In this case, the waking timestamp will be in the past
    //    because we added the descheduled slice when we processed the sched
    //    event).
    // 2. We're close to the end of the trace or had data-loss and we missed
    //    the switch out event for a thread but we see a waking after.

    // Case 1 described above. In this situation, we should drop the waking
    // entirely.
    if (table->ts()[it->second] >= ts) {
      return;
    }

    // Case 2 described above. We still want to set the thread as running but
    // we don't update the duration.
    UpdateDurIfNotRunning(ts, it->second, table);
  }

  // Add a dangling slice to represent that this slice is now available for
  // running. The duration will be updated by the next event (sched) that we
  // see.
  tables::ThreadStateTable::Row row;
  row.ts = ts;
  row.dur = -1;
  row.state = runnable_string_id_;
  row.utid = utid;
  state_map[utid] = table->Insert(row).row;
}

Table::Schema ThreadStateGenerator::CreateSchema() {
  auto schema = tables::ThreadStateTable::Schema();

  // Because we expect our users to generally want ordered by ts, we set the
  // ordering for the schema to match our forced sort pass in ComputeTable.
  auto ts_it = std::find_if(
      schema.columns.begin(), schema.columns.end(),
      [](const Table::Schema::Column& col) { return col.name == "ts"; });
  ts_it->is_sorted = true;

  return schema;
}

std::string ThreadStateGenerator::TableName() {
  return "thread_state";
}

uint32_t ThreadStateGenerator::EstimateRowCount() {
  return context_->storage->sched_slice_table().row_count();
}

}  // namespace trace_processor
}  // namespace perfetto
