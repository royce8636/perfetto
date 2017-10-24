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

#ifndef PROTOZERO_INCLUDE_PROTOZERO_SCATTERED_STREAM_WRITER_H_
#define PROTOZERO_INCLUDE_PROTOZERO_SCATTERED_STREAM_WRITER_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "protozero/contiguous_memory_range.h"

namespace protozero {

// This class deals with the following problem: append-only proto messages want
// to write a stream of bytes, without caring about the implementation of the
// underlying buffer (which concretely will be either the trace ring buffer
// or a heap-allocated buffer). The main deal is: proto messages don't know in
// advance what their size will be.
// Due to the tracing buffer being split into fixed-size chunks, on some
// occasions, these writes need to be spread over two (or more) non-contiguous
// chunks of memory. Similarly, when the buffer is backed by the heap, we want
// to avoid realloc() calls, as they might cause a full copy of the contents
// of the buffer.
// The purpose of this class is to abtract away the non-contiguous write logic.
// This class knows how to deal with writes as long as they fall in the same
// ContiguousMemoryRange and defers the chunk-chaining logic to the Delegate.
class ScatteredStreamWriter {
 public:
  class Delegate {
   public:
    virtual ~Delegate();
    virtual ContiguousMemoryRange GetNewBuffer() = 0;
  };

  explicit ScatteredStreamWriter(Delegate* delegate);
  ~ScatteredStreamWriter();

  inline void WriteByte(uint8_t value) {
    if (write_ptr_ >= cur_range_.end)
      Extend();
    *write_ptr_++ = value;
  }

  // Assumes that the caller checked that there is enough headroom.
  // TODO(primiano): perf optimization, this is a tracing hot path. The
  // compiler can make strong optimization on memcpy if the size arg is a
  // constexpr. Make a templated variant of this for fixed-size writes.
  // TODO(primiano): restrict / noalias might also help.
  inline void WriteBytesUnsafe(const uint8_t* src, size_t size) {
    uint8_t* const end = write_ptr_ + size;
    assert(end <= cur_range_.end);
    memcpy(write_ptr_, src, size);
    write_ptr_ = end;
  }

  inline void WriteBytes(const uint8_t* src, size_t size) {
    uint8_t* const end = write_ptr_ + size;
    if (__builtin_expect(end <= cur_range_.end, 1))
      return WriteBytesUnsafe(src, size);
    WriteBytesSlowPath(src, size);
  }

  void WriteBytesSlowPath(const uint8_t* src, size_t size);

  // Reserves a fixed amount of bytes to be backfilled later. The reserved range
  // is guaranteed to be contiguous and not span across chunks. |size| has to be
  // <= than the size of a new buffer returned by the Delegate::GetNewBuffer().
  ContiguousMemoryRange ReserveBytes(size_t size);

  // Fast (but unsafe) version of the above. The caller must have previously
  // checked that there are at least |size| contiguous bytes available.
  // Returns only the start pointer of the reservation.
  uint8_t* ReserveBytesUnsafe(size_t size) {
    uint8_t* begin = write_ptr_;
    write_ptr_ += size;
    assert(write_ptr_ <= cur_range_.end);
    return begin;
  }

  // Resets the buffer boundaries and the write pointer to the given |range|.
  // Subsequent WriteByte(s) will write into |range|.
  void Reset(ContiguousMemoryRange range);

  // Number of contiguous free bytes in |cur_range_| that can be written without
  // requesting a new buffer.
  size_t bytes_available() const {
    return static_cast<size_t>(cur_range_.end - write_ptr_);
  }

  uint8_t* write_ptr() const { return write_ptr_; }

 private:
  ScatteredStreamWriter(const ScatteredStreamWriter&) = delete;
  ScatteredStreamWriter& operator=(const ScatteredStreamWriter&) = delete;

  void Extend();

  Delegate* const delegate_;
  ContiguousMemoryRange cur_range_;
  uint8_t* write_ptr_;
};

}  // namespace protozero

#endif  // PROTOZERO_INCLUDE_PROTOZERO_SCATTERED_STREAM_WRITER_H_
