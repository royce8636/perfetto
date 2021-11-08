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

#ifndef INCLUDE_PERFETTO_TRACE_PROCESSOR_TRACE_BLOB_H_
#define INCLUDE_PERFETTO_TRACE_PROCESSOR_TRACE_BLOB_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "perfetto/base/build_config.h"
#include "perfetto/base/export.h"
#include "perfetto/base/logging.h"

// TODO(primiano): implement file mmap on Windows.
#if PERFETTO_BUILDFLAG(PERFETTO_OS_LINUX) || \
    PERFETTO_BUILDFLAG(PERFETTO_OS_APPLE) || \
    PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID)
#define TRACE_PROCESSOR_HAS_MMAP() 1
#else
#define TRACE_PROCESSOR_HAS_MMAP() 0
#endif

namespace perfetto {
namespace trace_processor {

// TraceBlob is a move-only buffer that owns a portion of memory containing
// trace data (not necessarily aligned at trace packet boundaries). Think of
// this as a std::pair<std::unique_ptr<uint8_t[]>, size_t>.
// TraceBlob can be instantiated and moved around when it's written/altered
// by the initial ingestion stages. In this mode, no refcounting is used
// (i.e. refcount_ is always == 0).
// When it comes to parsing stages, the TraceBlob can be turned into a read-only
// object wrapping it in a TraceBlobView. Once wrapped in a TraceBlobView, the
// TraceBlob becomes refcounted (TBV handles the inc/dec of refcount).
// TraceBlobView allows to have multiple instances pointing at (different
// sub-offsets of) the same TraceBlob.
// The neat thing about TraceBlob is that it deals transparently with owned
// memory (in the case of Allocate and TakeOwnership) and memory-mapped memory.
class PERFETTO_EXPORT TraceBlob {
 public:
  static TraceBlob Allocate(size_t size);
  static TraceBlob CopyFrom(const void*, size_t size);
  static TraceBlob TakeOwnership(std::unique_ptr<uint8_t[]>, size_t size);

  // Takes ownership of the mmap region. Will call munmap() on destruction.
  static TraceBlob FromMmap(void* data, size_t size);

  ~TraceBlob();

  // Allow move.
  TraceBlob(TraceBlob&& other) noexcept { *this = std::move(other); }
  TraceBlob& operator=(TraceBlob&&) noexcept;

  // Disallow copy.
  TraceBlob(const TraceBlob&) = delete;
  TraceBlob& operator=(const TraceBlob&) = delete;

  uint8_t* data() const { return data_; }
  size_t size() const { return size_; }

 protected:
  // "protected" here is not for inheritance. It's just to document the scope of
  // the methods exposed to TraceBlobView.
  friend class TraceBlobView;

  // Refcount inc/dec is used only by TraceBlobView.
  void IncRefcount() {
    PERFETTO_DCHECK(refcount_ >= 0);
    ++refcount_;
  }

  void DecRefcountAndDeleteIfZero() {
    PERFETTO_DCHECK(refcount_ > 0);
    if (--refcount_ == 0)
      delete this;
  }

 private:
  enum class Ownership { kNull = 0, kHeapBuf, kMmaped };

  TraceBlob(Ownership ownership, uint8_t* data, size_t size)
      : ownership_(ownership), data_(data), size_(size) {}

  int refcount_ = 0;
  Ownership ownership_ = Ownership::kNull;
  uint8_t* data_ = nullptr;
  size_t size_ = 0;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACE_PROCESSOR_TRACE_BLOB_H_
