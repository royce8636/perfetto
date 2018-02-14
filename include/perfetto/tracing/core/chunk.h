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

#ifndef INCLUDE_PERFETTO_TRACING_CORE_CHUNK_H_
#define INCLUDE_PERFETTO_TRACING_CORE_CHUNK_H_

#include <stddef.h>

#include <memory>
#include <vector>

namespace perfetto {

// A simple wrapper around a virtually contiguous memory range that contains a
// TracePacket, or just a portion of it.
struct Chunk {
  Chunk() : start(nullptr), size(0) {}
  Chunk(const void* st, size_t sz) : start(st), size(sz) {}
  Chunk(Chunk&& other) noexcept = default;

  // Create a Chunk which contains (and owns) a copy of the given memory.
  static Chunk Copy(const void* start, size_t size) {
    Chunk c;
    c.own_data_.reset(new uint8_t[size]);
    c.size = size;
    c.start = &c.own_data_[0];
    memcpy(&c.own_data_[0], start, size);
    return c;
  }

  const void* start;
  size_t size;

 private:
  Chunk(const Chunk&) = delete;
  void operator=(const Chunk&) = delete;

  std::unique_ptr<uint8_t[]> own_data_;
};

// TODO(primiano): most TracePacket(s) fit in a chunk or two. We need something
// a bit more clever here that has inline capacity for 2 chunks and then uses a
// std::forward_list or a std::vector for the less likely cases.
using ChunkSequence = std::vector<Chunk>;

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_CHUNK_H_
