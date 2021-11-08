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

#include "perfetto/trace_processor/trace_blob.h"

#include <stdlib.h>
#include <string.h>

#include "perfetto/base/logging.h"
#include "perfetto/ext/base/utils.h"
#include "perfetto/trace_processor/basic_types.h"

namespace perfetto {
namespace trace_processor {

// static
TraceBlob TraceBlob::Allocate(size_t size) {
  TraceBlob blob(Ownership::kHeapBuf, new uint8_t[size], size);
  PERFETTO_CHECK(blob.data_);
  return blob;
}

// static
TraceBlob TraceBlob::CopyFrom(const void* src, size_t size) {
  TraceBlob blob = Allocate(size);
  memcpy(blob.data_, src, size);
  return blob;
}

// static
TraceBlob TraceBlob::TakeOwnership(std::unique_ptr<uint8_t[]> buf,
                                   size_t size) {
  PERFETTO_CHECK(buf);
  return TraceBlob(Ownership::kHeapBuf, buf.release(), size);
}

TraceBlob::~TraceBlob() {
  PERFETTO_CHECK(refcount_ == 0);
  switch (ownership_) {
    case Ownership::kHeapBuf:
      delete[] data_;
      break;

    case Ownership::kNull:
      // Nothing to do.
      break;
  }
  data_ = nullptr;
  size_ = 0;
}

TraceBlob& TraceBlob::operator=(TraceBlob&& other) noexcept {
  if (this == &other)
    return *this;
  static_assert(sizeof(*this) == base::AlignUp<sizeof(void*)>(
                                     sizeof(data_) + sizeof(size_) +
                                     sizeof(ownership_) + sizeof(refcount_)),
                "TraceBlob move operator needs updating");
  data_ = other.data_;
  size_ = other.size_;
  ownership_ = other.ownership_;
  refcount_ = other.refcount_;
  other.refcount_ = 0;
  other.data_ = nullptr;
  other.size_ = 0;
  other.ownership_ = Ownership::kNull;
  return *this;
}

}  // namespace trace_processor
}  // namespace perfetto
