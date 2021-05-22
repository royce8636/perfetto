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

#include "perfetto/trace_processor/read_trace.h"

#include "perfetto/base/logging.h"
#include "perfetto/ext/base/file_utils.h"
#include "perfetto/ext/base/scoped_file.h"
#include "perfetto/ext/base/utils.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/trace_processor/trace_processor.h"

#include "src/trace_processor/forwarding_trace_parser.h"
#include "src/trace_processor/importers/gzip/gzip_trace_parser.h"
#include "src/trace_processor/importers/proto/proto_trace_tokenizer.h"
#include "src/trace_processor/util/gzip_utils.h"
#include "src/trace_processor/util/status_macros.h"

#include "protos/perfetto/trace/trace.pbzero.h"
#include "protos/perfetto/trace/trace_packet.pbzero.h"

#if PERFETTO_BUILDFLAG(PERFETTO_OS_LINUX) || \
    PERFETTO_BUILDFLAG(PERFETTO_OS_APPLE)
#define PERFETTO_HAS_AIO_H() 1
#else
#define PERFETTO_HAS_AIO_H() 0
#endif

#if PERFETTO_HAS_AIO_H()
#include <aio.h>
#endif

namespace perfetto {
namespace trace_processor {
namespace {

// 1MB chunk size seems the best tradeoff on a MacBook Pro 2013 - i7 2.8 GHz.
constexpr size_t kChunkSize = 1024 * 1024;

util::Status ReadTraceUsingRead(
    TraceProcessor* tp,
    int fd,
    uint64_t* file_size,
    const std::function<void(uint64_t parsed_size)>& progress_callback) {
  // Load the trace in chunks using ordinary read().
  for (int i = 0;; i++) {
    if (progress_callback && i % 128 == 0)
      progress_callback(*file_size);

    std::unique_ptr<uint8_t[]> buf(new uint8_t[kChunkSize]);
    auto rsize = base::Read(fd, buf.get(), kChunkSize);
    if (rsize == 0)
      break;

    if (rsize < 0) {
      return util::ErrStatus("Reading trace file failed (errno: %d, %s)", errno,
                             strerror(errno));
    }

    *file_size += static_cast<uint64_t>(rsize);

    RETURN_IF_ERROR(tp->Parse(std::move(buf), static_cast<size_t>(rsize)));
  }
  return util::OkStatus();
}

class SerializingProtoTraceReader : public ChunkedTraceReader {
 public:
  SerializingProtoTraceReader(std::vector<uint8_t>* output) : output_(output) {}

  util::Status Parse(std::unique_ptr<uint8_t[]> data, size_t size) override {
    return tokenizer_.Tokenize(
        std::move(data), size, [this](TraceBlobView packet) {
          uint8_t buffer[protozero::proto_utils::kMaxSimpleFieldEncodedSize];

          uint8_t* pos = buffer;
          pos = protozero::proto_utils::WriteVarInt(kTracePacketTag, pos);
          pos = protozero::proto_utils::WriteVarInt(packet.length(), pos);
          output_->insert(output_->end(), buffer, pos);

          output_->insert(output_->end(), packet.data(),
                          packet.data() + packet.length());
          return util::OkStatus();
        });
  }

  void NotifyEndOfFile() override {}

 private:
  static constexpr uint8_t kTracePacketTag =
      protozero::proto_utils::MakeTagLengthDelimited(
          protos::pbzero::Trace::kPacketFieldNumber);

  ProtoTraceTokenizer tokenizer_;
  std::vector<uint8_t>* output_;
};

}  // namespace

util::Status ReadTrace(
    TraceProcessor* tp,
    const char* filename,
    const std::function<void(uint64_t parsed_size)>& progress_callback) {
  base::ScopedFile fd(base::OpenFile(filename, O_RDONLY));
  if (!fd)
    return util::ErrStatus("Could not open trace file (path: %s)", filename);

  uint64_t file_size = 0;

#if PERFETTO_HAS_AIO_H()
  // Load the trace in chunks using async IO. We create a simple pipeline where,
  // at each iteration, we parse the current chunk and asynchronously start
  // reading the next chunk.
  struct aiocb cb {};
  cb.aio_nbytes = kChunkSize;
  cb.aio_fildes = *fd;

  std::unique_ptr<uint8_t[]> aio_buf(new uint8_t[kChunkSize]);
#if defined(MEMORY_SANITIZER)
  // Just initialize the memory to make the memory sanitizer happy as it
  // cannot track aio calls below.
  memset(aio_buf.get(), 0, kChunkSize);
#endif  // defined(MEMORY_SANITIZER)
  cb.aio_buf = aio_buf.get();

  PERFETTO_CHECK(aio_read(&cb) == 0);
  struct aiocb* aio_list[1] = {&cb};

  for (int i = 0;; i++) {
    if (progress_callback && i % 128 == 0)
      progress_callback(file_size);

    // Block waiting for the pending read to complete.
    PERFETTO_CHECK(aio_suspend(aio_list, 1, nullptr) == 0);
    auto rsize = aio_return(&cb);
    if (rsize <= 0)
      break;
    file_size += static_cast<uint64_t>(rsize);

    // Take ownership of the completed buffer and enqueue a new async read
    // with a fresh buffer.
    std::unique_ptr<uint8_t[]> buf(std::move(aio_buf));
    aio_buf.reset(new uint8_t[kChunkSize]);
#if defined(MEMORY_SANITIZER)
    // Just initialize the memory to make the memory sanitizer happy as it
    // cannot track aio calls below.
    memset(aio_buf.get(), 0, kChunkSize);
#endif  // defined(MEMORY_SANITIZER)
    cb.aio_buf = aio_buf.get();
    cb.aio_offset += rsize;
    PERFETTO_CHECK(aio_read(&cb) == 0);

    // Parse the completed buffer while the async read is in-flight.
    RETURN_IF_ERROR(tp->Parse(std::move(buf), static_cast<size_t>(rsize)));
  }

  if (file_size == 0) {
    PERFETTO_ILOG(
        "Failed to read any data using AIO. This is expected and not an error "
        "on WSL. Falling back to read()");
    RETURN_IF_ERROR(ReadTraceUsingRead(tp, *fd, &file_size, progress_callback));
  }
#else   // PERFETTO_HAS_AIO_H()
  RETURN_IF_ERROR(ReadTraceUsingRead(tp, *fd, &file_size, progress_callback));
#endif  // PERFETTO_HAS_AIO_H()

  tp->NotifyEndOfFile();
  tp->SetCurrentTraceName(filename);

  if (progress_callback)
    progress_callback(file_size);
  return util::OkStatus();
}

util::Status DecompressTrace(const uint8_t* data,
                             size_t size,
                             std::vector<uint8_t>* output) {
  TraceType type = GuessTraceType(data, size);
  if (type != TraceType::kGzipTraceType && type != TraceType::kProtoTraceType) {
    return util::ErrStatus(
        "Only GZIP and proto trace types are supported by DecompressTrace");
  }

  if (type == TraceType::kGzipTraceType) {
    std::unique_ptr<ChunkedTraceReader> reader(
        new SerializingProtoTraceReader(output));
    GzipTraceParser parser(std::move(reader));

    RETURN_IF_ERROR(parser.ParseUnowned(data, size));
    if (parser.needs_more_input())
      return util::ErrStatus("Cannot decompress partial trace file");

    parser.NotifyEndOfFile();
    return util::OkStatus();
  }

  PERFETTO_CHECK(type == TraceType::kProtoTraceType);

  protos::pbzero::Trace::Decoder decoder(data, size);
  util::GzipDecompressor decompressor;
  if (size > 0 && !decoder.packet()) {
    return util::ErrStatus("Trace does not contain valid packets");
  }
  for (auto it = decoder.packet(); it; ++it) {
    protos::pbzero::TracePacket::Decoder packet(*it);
    if (!packet.has_compressed_packets()) {
      it->SerializeAndAppendTo(output);
      continue;
    }

    // Make sure that to reset the stream between the gzip streams.
    auto bytes = packet.compressed_packets();
    decompressor.Reset();
    decompressor.SetInput(bytes.data, bytes.size);

    using ResultCode = util::GzipDecompressor::ResultCode;
    uint8_t out[4096];
    for (auto ret = ResultCode::kOk; ret != ResultCode::kEof;) {
      auto res = decompressor.Decompress(out, base::ArraySize(out));
      ret = res.ret;
      if (ret == ResultCode::kError || ret == ResultCode::kNoProgress ||
          ret == ResultCode::kNeedsMoreInput) {
        return util::ErrStatus("Failed while decompressing stream");
      }
      output->insert(output->end(), out, out + res.bytes_written);
    }
  }
  return util::OkStatus();
}

}  // namespace trace_processor
}  // namespace perfetto
