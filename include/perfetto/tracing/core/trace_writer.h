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

#ifndef INCLUDE_PERFETTO_TRACING_CORE_TRACE_WRITER_H_
#define INCLUDE_PERFETTO_TRACING_CORE_TRACE_WRITER_H_

#include <functional>

#include "perfetto/base/export.h"
#include "perfetto/protozero/message_handle.h"
#include "perfetto/tracing/core/basic_types.h"

namespace perfetto {

namespace protos {
namespace pbzero {
class TracePacket;
}  // namespace pbzero
}  // namespace protos

// This is a single-thread write interface that allows to write protobufs
// directly into the tracing shared buffer without making any copies.
// It takes care of acquiring and releasing chunks from the
// SharedMemoryArbiter and splitting protos over chunks.
// The idea is that each data source creates one (or more) TraceWriter for each
// thread it wants to write from. Each TraceWriter will get its own dedicated
// chunk and will write into the shared buffer without any locking most of the
// time. Locking will happen only when a chunk is exhausted and a new one is
// acquired from the arbiter.

// TODO: TraceWriter needs to keep the shared memory buffer alive (refcount?).
// Otherwise if the shared memory buffer goes away (e.g. the Service crashes)
// the TraceWriter will keep writing into unmapped memory.

class PERFETTO_EXPORT TraceWriter {
 public:
  using TracePacketHandle =
      protozero::MessageHandle<protos::pbzero::TracePacket>;

  TraceWriter();
  virtual ~TraceWriter();

  // Returns a handle to the root proto message for the trace. The message will
  // be finalized either by calling directly handle.Finalize() or by letting the
  // handle go out of scope. The returned handle can be std::move()'d but cannot
  // be used after either: (i) the TraceWriter instance is destroyed, (ii) a
  // subsequence NewTracePacket() call is made on the same TraceWriter instance.
  virtual TracePacketHandle NewTracePacket() = 0;

  // Commits the data pending for the current chunk into the shared memory
  // buffer and sends a CommitDataRequest() to the service. This can be called
  // only if handle returned by NewTracePacket() has been destroyed (i.e. we
  // cannot Flush() while writing a TracePacket).
  // Note: Flush() also happens implicitly when destroying the TraceWriter.
  // |callback| is an optional callback. When non-null it will request the
  // service to ACK the flush and will be invoked after the service has
  // ackwnoledged it. Please note that the callback might be NEVER INVOKED, for
  // instance if the service crashes or the IPC connection is dropped. The
  // callback should be used only by tests and best-effort features (logging).
  // TODO(primiano): right now the |callback| will be called on the IPC thread.
  // This is fine in the current single-thread scenario, but long-term
  // trace_writer_impl.cc should be smarter and post it on the right thread.
  virtual void Flush(std::function<void()> callback = {}) = 0;

  virtual WriterID writer_id() const = 0;

  // Bytes written since creation. Is not reset when new chunks are acquired.
  virtual uint64_t written() const = 0;

  // Set the id of the first chunk the writer will emit. Returns |false| if not
  // implemented or if the first chunk was already emitted by the writer.
  //
  // StartupTraceWriter will call this if it committed buffered data on
  // behalf of the TraceWriter.
  virtual bool SetFirstChunkId(ChunkID);

 private:
  TraceWriter(const TraceWriter&) = delete;
  TraceWriter& operator=(const TraceWriter&) = delete;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_TRACE_WRITER_H_
