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

#include <stddef.h>
#include <stdint.h>

#include "perfetto/base/file_utils.h"
#include "perfetto/base/temp_file.h"
#include "src/profiling/memory/shared_ring_buffer.h"

namespace perfetto {
namespace profiling {
namespace {

struct MetadataHeader {
  std::atomic<bool> spinlock;
  uint64_t read_pos;
  uint64_t write_pos;
};

bool IsPow2(size_t x) {
  return (x & (x - 1)) == 0;
}

size_t RoundToPow2(size_t x) {
  if (IsPow2(x))
    return x;

  if (x == 1)
    return 2;
  return 2 * (x & (x - 1));
}

int FuzzRingBuffer(const uint8_t* data, size_t size) {
  if (size <= sizeof(MetadataHeader))
    return 0;

  auto fd = base::TempFile::CreateUnlinked().ReleaseFD();
  PERFETTO_CHECK(fd);
  PERFETTO_CHECK(base::WriteAll(*fd, data, sizeof(MetadataHeader)) != -1);
  PERFETTO_CHECK(lseek(*fd, base::kPageSize, SEEK_SET) != -1);

  // Put the remaining fuzzer input into the data portion of the ring buffer.
  size_t payload_size = size - sizeof(MetadataHeader);
  const uint8_t* payload = data + sizeof(MetadataHeader);
  size_t payload_size_pages =
      (payload_size + base::kPageSize - 1) / base::kPageSize;
  // Upsize test buffer to be 2^n data pages (precondition of the impl) + 1 page
  // for the metadata.
  size_t total_size_pages = 1 + RoundToPow2(payload_size_pages);
  PERFETTO_CHECK(base::WriteAll(*fd, payload, payload_size) != -1);
  if (base::kPageSize + payload_size != total_size_pages * base::kPageSize) {
    PERFETTO_CHECK(
        lseek(*fd, total_size_pages * base::kPageSize - 1, SEEK_SET) != -1);
    char null[1] = {'\0'};
    PERFETTO_CHECK(base::WriteAll(*fd, null, sizeof(null) != -1));
  }
  PERFETTO_CHECK(lseek(*fd, 0, SEEK_SET) != -1);

  auto buf = SharedRingBuffer::Attach(std::move(fd));
  PERFETTO_CHECK(!!buf);
  bool did_read;
  do {
    auto read_buf = buf->BeginRead();
    did_read = bool(read_buf);
    if (did_read) {
      volatile uint8_t* v_data = read_buf.data;
      // Assert we get a reference to valid memory.
      for (size_t i = 0; i < read_buf.size; ++i)
        v_data[i] = v_data[i];
    }
    buf->EndRead(std::move(read_buf));
  } while (did_read);
  return 0;
}

}  // namespace
}  // namespace profiling
}  // namespace perfetto

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return perfetto::profiling::FuzzRingBuffer(data, size);
}
