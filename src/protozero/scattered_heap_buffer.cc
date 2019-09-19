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

#include "perfetto/protozero/scattered_heap_buffer.h"

#include <algorithm>

namespace protozero {

ScatteredHeapBuffer::Slice::Slice(size_t size)
    : buffer_(std::unique_ptr<uint8_t[]>(new uint8_t[size])),
      size_(size),
      unused_bytes_(size) {
  PERFETTO_DCHECK(size);
#if PERFETTO_DCHECK_IS_ON()
  memset(start(), 0xff, size_);
#endif  // PERFETTO_DCHECK_IS_ON()
}

ScatteredHeapBuffer::Slice::Slice(Slice&& slice) noexcept = default;

ScatteredHeapBuffer::Slice::~Slice() = default;

ScatteredHeapBuffer::ScatteredHeapBuffer(size_t initial_slice_size_bytes,
                                         size_t maximum_slice_size_bytes)
    : next_slice_size_(initial_slice_size_bytes),
      maximum_slice_size_(maximum_slice_size_bytes) {
  PERFETTO_DCHECK(next_slice_size_ && maximum_slice_size_);
  PERFETTO_DCHECK(maximum_slice_size_ >= initial_slice_size_bytes);
}

ScatteredHeapBuffer::~ScatteredHeapBuffer() = default;

protozero::ContiguousMemoryRange ScatteredHeapBuffer::GetNewBuffer() {
  PERFETTO_CHECK(writer_);
  AdjustUsedSizeOfCurrentSlice();

  slices_.emplace_back(next_slice_size_);
  next_slice_size_ = std::min(maximum_slice_size_, next_slice_size_ * 2);
  return slices_.back().GetTotalRange();
}

std::vector<uint8_t> ScatteredHeapBuffer::StitchSlices() {
  AdjustUsedSizeOfCurrentSlice();
  std::vector<uint8_t> buffer;
  for (const auto& slice : slices_) {
    auto used_range = slice.GetUsedRange();
    buffer.insert(buffer.end(), used_range.begin, used_range.end);
  }
  return buffer;
}

void ScatteredHeapBuffer::AdjustUsedSizeOfCurrentSlice() {
  if (!slices_.empty())
    slices_.back().set_unused_bytes(writer_->bytes_available());
}

size_t ScatteredHeapBuffer::GetTotalSize() {
  size_t total_size = 0;
  for (auto& slice : slices_) {
    total_size += slice.size();
  }
  return total_size;
}

}  // namespace protozero
