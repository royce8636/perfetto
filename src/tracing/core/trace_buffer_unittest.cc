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

#include <string.h>

#include <initializer_list>
#include <random>
#include <sstream>
#include <vector>

#include "perfetto/protozero/proto_utils.h"
#include "perfetto/tracing/core/basic_types.h"
#include "perfetto/tracing/core/shared_memory_abi.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "src/tracing/core/trace_buffer.h"
#include "src/tracing/test/fake_packet.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace perfetto {

using ::testing::ContainerEq;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

class TraceBufferTest : public testing::Test {
 public:
  using SequenceIterator = TraceBuffez::SequenceIterator;
  using ChunkMetaKey = TraceBuffez::ChunkMeta::Key;
  using ChunkRecord = TraceBuffez::ChunkRecord;

  static constexpr uint8_t kContFromPrevChunk =
      SharedMemoryABI::ChunkHeader::kFirstPacketContinuesFromPrevChunk;
  static constexpr uint8_t kContOnNextChunk =
      SharedMemoryABI::ChunkHeader::kLastPacketContinuesOnNextChunk;
  static constexpr uint8_t kChunkNeedsPatching =
      SharedMemoryABI::ChunkHeader::kChunkNeedsPatching;

  FakeChunk CreateChunk(ProducerID p, WriterID w, ChunkID c) {
    return FakeChunk(trace_buffer_.get(), p, w, c);
  }

  void ResetBuffer(size_t size_) {
    trace_buffer_ = TraceBuffez::Create(size_);
    ASSERT_TRUE(trace_buffer_);
  }

  bool TryPatchChunkContents(ProducerID p,
                             WriterID w,
                             ChunkID c,
                             std::vector<TraceBuffez::Patch> patches,
                             bool other_patches_pending = false) {
    return trace_buffer_->TryPatchChunkContents(
        p, w, c, patches.data(), patches.size(), other_patches_pending);
  }

  std::vector<FakePacketFragment> ReadPacket(uint32_t* uid = nullptr) {
    std::vector<FakePacketFragment> fragments;
    TracePacket packet;
    uint32_t ignore;
    if (!trace_buffer_->ReadNextTracePacket(&packet, uid ? uid : &ignore))
      return fragments;
    for (const Slice& slice : packet.slices())
      fragments.emplace_back(slice.start, slice.size);
    return fragments;
  }

  void AppendChunks(
      std::initializer_list<std::tuple<ProducerID, WriterID, ChunkID>> chunks) {
    for (const auto& c : chunks) {
      char seed =
          static_cast<char>(std::get<0>(c) + std::get<1>(c) + std::get<2>(c));
      CreateChunk(std::get<0>(c), std::get<1>(c), std::get<2>(c))
          .AddPacket(4, seed)
          .CopyIntoTraceBuffer();
    }
  }

  bool IteratorSeqEq(ProducerID p,
                     WriterID w,
                     std::initializer_list<ChunkID> chunk_ids) {
    std::stringstream expected_seq;
    for (const auto& c : chunk_ids)
      expected_seq << "{" << p << "," << w << "," << c << "},";

    std::stringstream actual_seq;
    for (auto it = GetReadIterForSequence(p, w); it.is_valid(); it.MoveNext()) {
      actual_seq << "{" << it.producer_id() << "," << it.writer_id() << ","
                 << it.chunk_id() << "},";
    }
    std::string expected_seq_str = expected_seq.str();
    std::string actual_seq_str = actual_seq.str();
    EXPECT_EQ(expected_seq_str, actual_seq_str);
    return expected_seq_str == actual_seq_str;
  }

  SequenceIterator GetReadIterForSequence(ProducerID p, WriterID w) {
    TraceBuffez::ChunkMeta::Key key(p, w, 0);
    return trace_buffer_->GetReadIterForSequence(
        trace_buffer_->index_.lower_bound(key));
  }

  void SuppressSanityDchecksForTesting() {
    trace_buffer_->suppress_sanity_dchecks_for_testing_ = true;
  }

  std::vector<ChunkMetaKey> GetIndex() {
    std::vector<ChunkMetaKey> keys;
    keys.reserve(trace_buffer_->index_.size());
    for (const auto& it : trace_buffer_->index_)
      keys.push_back(it.first);
    return keys;
  }

  TraceBuffez* trace_buffer() { return trace_buffer_.get(); }
  size_t size_to_end() { return trace_buffer_->size_to_end(); }

 private:
  std::unique_ptr<TraceBuffez> trace_buffer_;
};

// ----------------------
// Main TraceBuffer tests
// ----------------------

// Note for the test code: remember that the resulting size of a chunk is:
// SUM(packets) + 16 (that is sizeof(ChunkRecord)).
// Also remember that chunks are rounded up to 16. So, unless we are testing the
// rounding logic, might be a good idea to create chunks of that size.

TEST_F(TraceBufferTest, ReadWrite_EmptyBuffer) {
  ResetBuffer(4096);
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// On each iteration writes a fixed-size chunk and reads it back.
TEST_F(TraceBufferTest, ReadWrite_Simple) {
  ResetBuffer(64 * 1024);
  for (ChunkID chunk_id = 0; chunk_id < 1000; chunk_id++) {
    char seed = static_cast<char>(chunk_id);
    CreateChunk(ProducerID(1), WriterID(1), chunk_id)
        .AddPacket(42, seed)
        .CopyIntoTraceBuffer();
    trace_buffer()->BeginRead();
    ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(42, seed)));
    ASSERT_THAT(ReadPacket(), IsEmpty());
  }
}

TEST_F(TraceBufferTest, ReadWrite_OneChunkPerWriter) {
  for (uint8_t num_writers = 1; num_writers <= 10; num_writers++) {
    ResetBuffer(4096);
    for (uint8_t i = 1; i <= num_writers; i++) {
      ASSERT_EQ(32u, CreateChunk(ProducerID(i), WriterID(i), ChunkID(i))
                         .AddPacket(32 - 16, i)
                         .CopyIntoTraceBuffer());
    }

    // The expected read sequence now is: c3, c4, c5.
    trace_buffer()->BeginRead();
    for (uint8_t i = 1; i <= num_writers; i++)
      ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(32 - 16, i)));
    ASSERT_THAT(ReadPacket(), IsEmpty());
  }  // for(num_writers)
}

// Writes chunk that up filling the buffer precisely until the end, like this:
// [ c0: 512 ][ c1: 512 ][ c2: 1024 ][ c3: 2048 ]
// | ---------------- 4k buffer --------------- |
TEST_F(TraceBufferTest, ReadWrite_FillTillEnd) {
  ResetBuffer(4096);
  for (int i = 0; i < 3; i++) {
    ASSERT_EQ(512u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
                        .AddPacket(512 - 16, 'a')
                        .CopyIntoTraceBuffer());
    ASSERT_EQ(512u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
                        .AddPacket(512 - 16, 'b')
                        .CopyIntoTraceBuffer());
    ASSERT_EQ(1024u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
                         .AddPacket(1024 - 16, 'c')
                         .CopyIntoTraceBuffer());
    ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
                         .AddPacket(2048 - 16, 'd')
                         .CopyIntoTraceBuffer());

    // At this point the write pointer should have been reset at the beginning.
    ASSERT_EQ(4096u, size_to_end());

    trace_buffer()->BeginRead();
    ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(512 - 16, 'a')));
    ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(512 - 16, 'b')));
    ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(1024 - 16, 'c')));
    ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(2048 - 16, 'd')));
    ASSERT_THAT(ReadPacket(), IsEmpty());
  }
}

// Similar to the above, but this time leaves some gap at the end and then
// tries to add a chunk that doesn't fit to exercise the padding-at-end logic.
// Initial condition:
// [ c0: 128 ][ c1: 256 ][ c2: 512   ][ c3: 1024 ][ c4: 2048 ]{ 128 padding }
// | ------------------------------- 4k buffer ------------------------------ |
//
// At this point we try to insert a 512 Bytes chunk (c5). The result should be:
// [ c5: 512              ]{ padding }[c3: 1024 ][ c4: 2048 ]{ 128 padding }
// | ------------------------------- 4k buffer ------------------------------ |
TEST_F(TraceBufferTest, ReadWrite_Padding) {
  ResetBuffer(4096);
  ASSERT_EQ(128u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
                      .AddPacket(128 - 16, 'a')
                      .CopyIntoTraceBuffer());
  ASSERT_EQ(256u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
                      .AddPacket(256 - 16, 'b')
                      .CopyIntoTraceBuffer());
  ASSERT_EQ(512u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
                      .AddPacket(512 - 16, 'c')
                      .CopyIntoTraceBuffer());
  ASSERT_EQ(1024u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
                       .AddPacket(1024 - 16, 'd')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(4))
                       .AddPacket(2048 - 16, 'e')
                       .CopyIntoTraceBuffer());

  // Now write c5 that will cause wrapping + padding.
  ASSERT_EQ(128u, size_to_end());
  ASSERT_EQ(512u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(5))
                      .AddPacket(512 - 16, 'f')
                      .CopyIntoTraceBuffer());
  ASSERT_EQ(4096u - 512, size_to_end());

  // The expected read sequence now is: c3, c4, c5.
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(1024 - 16, 'd')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(2048 - 16, 'e')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(512 - 16, 'f')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Like ReadWrite_Padding, but this time the padding introduced is the minimum
// allowed (16 bytes). This is to exercise edge cases in the padding logic.
// [c0: 2048               ][c1: 1024         ][c2: 1008       ][c3: 16]
// [c4: 2032            ][c5: 1040                ][c6 :16][c7: 1080   ]
TEST_F(TraceBufferTest, ReadWrite_MinimalPadding) {
  ResetBuffer(4096);

  ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
                       .AddPacket(2048 - 16, 'a')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(1024u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
                       .AddPacket(1024 - 16, 'b')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(1008u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
                       .AddPacket(1008 - 16, 'c')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(16u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
                     .CopyIntoTraceBuffer());

  ASSERT_EQ(4096u, size_to_end());

  ASSERT_EQ(2032u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(4))
                       .AddPacket(2032 - 16, 'd')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(1040u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(5))
                       .AddPacket(1040 - 16, 'e')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(16u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(6))
                     .CopyIntoTraceBuffer());
  ASSERT_EQ(1008u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(7))
                       .AddPacket(1008 - 16, 'f')
                       .CopyIntoTraceBuffer());

  ASSERT_EQ(4096u, size_to_end());

  // The expected read sequence now is: c3, c4, c5.
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(2032 - 16, 'd')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(1040 - 16, 'e')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(1008 - 16, 'f')));
  for (int i = 0; i < 3; i++)
    ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, ReadWrite_RandomChunksNoWrapping) {
  for (int seed = 1; seed <= 32; seed++) {
    std::minstd_rand0 rnd_engine(seed);
    ResetBuffer(4096 * (1 + rnd_engine() % 32));
    std::uniform_int_distribution<size_t> size_dist(18, 4096);
    std::uniform_int_distribution<ProducerID> prod_dist(1, kMaxProducerID);
    std::uniform_int_distribution<WriterID> wri_dist(1, kMaxWriterID);
    ChunkID chunk_id = 0;
    std::map<std::tuple<ProducerID, WriterID, ChunkID>, size_t> expected_chunks;
    for (;;) {
      const size_t chunk_size = size_dist(rnd_engine);
      if (base::AlignUp<16>(chunk_size) >= size_to_end())
        break;
      ProducerID p = prod_dist(rnd_engine);
      WriterID w = wri_dist(rnd_engine);
      ChunkID c = chunk_id++;
      expected_chunks.emplace(std::make_tuple(p, w, c), chunk_size);
      ASSERT_EQ(chunk_size,
                CreateChunk(p, w, c)
                    .AddPacket(chunk_size - 16, static_cast<char>(chunk_size))
                    .CopyIntoTraceBuffer());
    }  // for(;;)
    trace_buffer()->BeginRead();
    for (const auto& it : expected_chunks) {
      const size_t chunk_size = it.second;
      ASSERT_THAT(ReadPacket(),
                  ElementsAre(FakePacketFragment(
                      chunk_size - 16, static_cast<char>(chunk_size))));
    }
    ASSERT_THAT(ReadPacket(), IsEmpty());
  }
}

// Tests the case of writing a chunk that leaves just sizeof(ChunkRecord) at
// the end of the buffer.
TEST_F(TraceBufferTest, ReadWrite_WrappingCases) {
  ResetBuffer(4096);
  ASSERT_EQ(4080u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
                       .AddPacket(4080 - 16, 'a')
                       .CopyIntoTraceBuffer());
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4080 - 16, 'a')));
  ASSERT_THAT(ReadPacket(), IsEmpty());

  ASSERT_EQ(16u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
                     .CopyIntoTraceBuffer());
  ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
                       .AddPacket(2048 - 16, 'b')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
                       .AddPacket(2048 - 16, 'c')
                       .CopyIntoTraceBuffer());
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(2048 - 16, 'b')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(2048 - 16, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Tests that when records are removed when adding padding at the end because
// there is no space left. The scenario is the following:
// Initial condition: [ c0: 2048 ][ c1: 2048 ]
// 2nd iteration:     [ c2: 2048] <-- write pointer is here
// At this point we try to add a 3072 bytes chunk. It won't fit because the
// space left till the end is just 2048 bytes. At this point we expect that a
// padding record is added in place of c1, and c1 is removed from the index.
// Final situation:   [ c3: 3072     ][ PAD ]
TEST_F(TraceBufferTest, ReadWrite_PaddingAtEndUpdatesIndex) {
  ResetBuffer(4096);
  // Setup initial condition: [ c0: 2048 ][ c1: 2048 ]
  ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
                       .AddPacket(2048 - 16, 'a')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
                       .AddPacket(2048 - 16, 'b')
                       .CopyIntoTraceBuffer());
  ASSERT_THAT(GetIndex(),
              ElementsAre(ChunkMetaKey(1, 1, 0), ChunkMetaKey(1, 1, 1)));

  // Wrap and get to this: [ c2: 2048] <-- write pointer is here
  ASSERT_EQ(2048u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
                       .AddPacket(2048 - 16, 'c')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(2048u, size_to_end());
  ASSERT_THAT(GetIndex(),
              ElementsAre(ChunkMetaKey(1, 1, 1), ChunkMetaKey(1, 1, 2)));

  // Force wrap because of lack of space and get: [ c3: 3072     ][ PAD ].
  ASSERT_EQ(3072u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
                       .AddPacket(3072 - 16, 'd')
                       .CopyIntoTraceBuffer());
  ASSERT_THAT(GetIndex(), ElementsAre(ChunkMetaKey(1, 1, 3)));

  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(3072 - 16, 'd')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Similar to ReadWrite_PaddingAtEndUpdatesIndex but makes it so that the
// various chunks don't perfectly align when wrapping.
TEST_F(TraceBufferTest, ReadWrite_PaddingAtEndUpdatesIndexMisaligned) {
  ResetBuffer(4096);

  // [c0: 512][c1: 512][c2: 512][c3: 512][c4: 512][c5: 512][c6: 512][c7: 512]
  for (uint8_t i = 0; i < 8; i++) {
    ASSERT_EQ(512u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(i))
                        .AddPacket(512 - 16, 'a' + i)
                        .CopyIntoTraceBuffer());
  }
  ASSERT_EQ(8u, GetIndex().size());

  // [c8: 2080..........................][PAD][c5: 512][c6: 512][c7: 512]
  //                                     ^ write pointer is here.
  ASSERT_EQ(2080u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(8))
                       .AddPacket(2080 - 16, 'i')
                       .CopyIntoTraceBuffer());
  ASSERT_EQ(2016u, size_to_end());
  ASSERT_THAT(GetIndex(),
              ElementsAre(ChunkMetaKey(1, 1, 5), ChunkMetaKey(1, 1, 6),
                          ChunkMetaKey(1, 1, 7), ChunkMetaKey(1, 1, 8)));

  // [ c9: 3104....................................][ PAD...............].
  ASSERT_EQ(3104u, CreateChunk(ProducerID(1), WriterID(1), ChunkID(9))
                       .AddPacket(3104 - 16, 'j')
                       .CopyIntoTraceBuffer());
  ASSERT_THAT(GetIndex(), ElementsAre(ChunkMetaKey(1, 1, 9)));

  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(3104u - 16, 'j')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// --------------------------------------
// Fragments stitching and skipping logic
// --------------------------------------

TEST_F(TraceBufferTest, Fragments_Simple) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(10, 'a', kContFromPrevChunk)
      .AddPacket(20, 'b')
      .AddPacket(30, 'c')
      .AddPacket(10, 'd', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(20, 'e', kContFromPrevChunk)
      .AddPacket(30, 'f')
      .CopyIntoTraceBuffer();

  trace_buffer()->BeginRead();
  // The (10, 'a') entry should be skipped because we don't have provided the
  // previous chunk, hence should be treated as a data loss.
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(20, 'b')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(30, 'c')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(10, 'd'),
                                        FakePacketFragment(20, 'e')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(30, 'f')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Fragments_EdgeCases) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(2, 'a', kContFromPrevChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(2, 'b', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), IsEmpty());

  // Now add the missing fragment.
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
      .AddPacket(2, 'c', kContFromPrevChunk)
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(2, 'b'),
                                        FakePacketFragment(2, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Receive packet fragments for the sequence {1,1} in the chunk order {0,2,1}
// and verify that they still get realigned properly, without breaking other
// sequences.
TEST_F(TraceBufferTest, Fragments_OutOfOrder) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(10, 'a', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
      .AddPacket(30, 'c', kContFromPrevChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(2), ChunkID(0))
      .AddPacket(10, 'd')
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(10, 'd')));
  ASSERT_THAT(ReadPacket(), IsEmpty());

  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(20, 'b', kContFromPrevChunk | kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
      .AddPacket(40, 'd')
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(10, 'a'),
                                        FakePacketFragment(20, 'b'),
                                        FakePacketFragment(30, 'c')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(40, 'd')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Fragments_EmptyChunkBefore) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0)).CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(10, 'a')
      .AddPacket(20, 'b', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
      .AddPacket(30, 'c', kContFromPrevChunk)
      .AddPacket(40, 'd', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(10, 'a')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(20, 'b'),
                                        FakePacketFragment(30, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Fragments_EmptyChunkAfter) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(10, 'a')
      .AddPacket(10, 'b', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1)).CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(10, 'a')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Set up a fragmented packet that happens to also have an empty chunk in the
// middle of the sequence. Test that it just gets skipped.
TEST_F(TraceBufferTest, Fragments_EmptyChunkInTheMiddle) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(10, 'a', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1)).CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
      .AddPacket(10, 'b', kContFromPrevChunk)
      .AddPacket(20, 'c')
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(10, 'a'),
                                        FakePacketFragment(10, 'b')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(20, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Generates sequences of fragmented packets of increasing length (|seq_len|),
// from [P0, P1a][P1y] to [P0, P1a][P1b][P1c]...[P1y]. Test that they are always
// read as one packet.
TEST_F(TraceBufferTest, Fragments_LongPackets) {
  for (unsigned seq_len = 1; seq_len <= 10; seq_len++) {
    ResetBuffer(4096);
    std::vector<FakePacketFragment> expected_fragments;
    expected_fragments.emplace_back(20, 'b');
    CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
        .AddPacket(10, 'a')
        .AddPacket(20, 'b', kContOnNextChunk)
        .CopyIntoTraceBuffer();
    for (unsigned i = 1; i <= seq_len; i++) {
      char prefix = 'b' + static_cast<char>(i);
      expected_fragments.emplace_back(20 + i, prefix);
      CreateChunk(ProducerID(1), WriterID(1), ChunkID(i))
          .AddPacket(20 + i, prefix, kContFromPrevChunk | kContOnNextChunk)
          .CopyIntoTraceBuffer();
    }
    expected_fragments.emplace_back(30, 'y');
    CreateChunk(ProducerID(1), WriterID(1), ChunkID(seq_len + 1))
        .AddPacket(30, 'y', kContFromPrevChunk)
        .AddPacket(50, 'z')
        .CopyIntoTraceBuffer();

    trace_buffer()->BeginRead();
    ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(10, 'a')));
    ASSERT_THAT(ReadPacket(), ContainerEq(expected_fragments));
    ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(50, 'z')));
    ASSERT_THAT(ReadPacket(), IsEmpty());
  }
}

// Similar to Fragments_LongPacket, but covers also the case of ChunkID wrapping
// over its max value.
TEST_F(TraceBufferTest, Fragments_LongPacketWithWrappingID) {
  ResetBuffer(4096);
  std::vector<FakePacketFragment> expected_fragments;

  for (ChunkID chunk_id = -2; chunk_id <= 2; chunk_id++) {
    char prefix = static_cast<char>('c' + chunk_id);
    expected_fragments.emplace_back(10 + chunk_id, prefix);
    CreateChunk(ProducerID(1), WriterID(1), chunk_id)
        .AddPacket(10 + chunk_id, prefix, kContOnNextChunk)
        .CopyIntoTraceBuffer();
  }
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ContainerEq(expected_fragments));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Fragments_PreserveUID) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(10, 'a')
      .AddPacket(10, 'b', kContOnNextChunk)
      .SetUID(11)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(0))
      .AddPacket(10, 'c')
      .AddPacket(10, 'd')
      .SetUID(22)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(10, 'e', kContFromPrevChunk)
      .AddPacket(10, 'f')
      .SetUID(11)
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  uid_t uid = -1;
  ASSERT_THAT(ReadPacket(&uid), ElementsAre(FakePacketFragment(10, 'a')));
  ASSERT_EQ(11u, uid);

  ASSERT_THAT(ReadPacket(&uid), ElementsAre(FakePacketFragment(10, 'b'),
                                            FakePacketFragment(10, 'e')));
  ASSERT_EQ(11u, uid);

  ASSERT_THAT(ReadPacket(&uid), ElementsAre(FakePacketFragment(10, 'f')));
  ASSERT_EQ(11u, uid);

  ASSERT_THAT(ReadPacket(&uid), ElementsAre(FakePacketFragment(10, 'c')));
  ASSERT_EQ(22u, uid);

  ASSERT_THAT(ReadPacket(&uid), ElementsAre(FakePacketFragment(10, 'd')));
  ASSERT_EQ(22u, uid);

  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// --------------------------
// Out of band patching tests
// --------------------------

TEST_F(TraceBufferTest, Patching_Simple) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(100, 'a')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(0))
      .AddPacket(9, 'b')
      .ClearBytes(5, 4)  // 5 := 4th payload byte. Byte 0 is the varint header.
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(3), WriterID(1), ChunkID(0))
      .AddPacket(100, 'c')
      .CopyIntoTraceBuffer();
  ASSERT_TRUE(TryPatchChunkContents(ProducerID(2), WriterID(1), ChunkID(0),
                                    {{5, {{'Y', 'M', 'C', 'A'}}}}));
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(100, 'a')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment("b00-YMCA", 8)));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(100, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Patching_SkipIfChunkDoesntExist) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(100, 'a')
      .CopyIntoTraceBuffer();
  ASSERT_FALSE(TryPatchChunkContents(ProducerID(1), WriterID(2), ChunkID(0),
                                     {{0, {{'X', 'X', 'X', 'X'}}}}));
  ASSERT_FALSE(TryPatchChunkContents(ProducerID(1), WriterID(1), ChunkID(1),
                                     {{0, {{'X', 'X', 'X', 'X'}}}}));
  ASSERT_FALSE(TryPatchChunkContents(ProducerID(1), WriterID(1), ChunkID(-1),
                                     {{0, {{'X', 'X', 'X', 'X'}}}}));
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(100, 'a')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Patching_AtBoundariesOfChunk) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(100, 'a', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(16, 'b', kContFromPrevChunk | kContOnNextChunk)
      .ClearBytes(1, 4)
      .ClearBytes(16 - 4, 4)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
      .AddPacket(100, 'c', kContFromPrevChunk)
      .CopyIntoTraceBuffer();
  ASSERT_TRUE(TryPatchChunkContents(
      ProducerID(1), WriterID(1), ChunkID(1),
      {{1, {{'P', 'E', 'R', 'F'}}}, {16 - 4, {{'E', 'T', 'T', 'O'}}}}));
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(),
              ElementsAre(FakePacketFragment(100, 'a'),
                          FakePacketFragment("PERFb01-b02ETTO", 15),
                          FakePacketFragment(100, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Tests kChunkNeedsPatching logic: chunks that are marked as "pending patch"
// should not be read until the patch has happened.
TEST_F(TraceBufferTest, Patching_ReadWaitsForPatchComplete) {
  ResetBuffer(4096);

  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(16, 'a', kChunkNeedsPatching)
      .ClearBytes(1, 4)  // 1 := 0th payload byte. Byte 0 is the varint header.
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(16, 'b')
      .CopyIntoTraceBuffer();

  CreateChunk(ProducerID(2), WriterID(1), ChunkID(0))
      .AddPacket(16, 'c')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(1))
      .AddPacket(16, 'd', kChunkNeedsPatching)
      .ClearBytes(1, 4)  // 1 := 0th payload byte. Byte 0 is the varint header.
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(2))
      .AddPacket(16, 'e')
      .CopyIntoTraceBuffer();

  CreateChunk(ProducerID(3), WriterID(1), ChunkID(0))
      .AddPacket(16, 'f', kChunkNeedsPatching)
      .ClearBytes(1, 8)  // 1 := 0th payload byte. Byte 0 is the varint header.
      .CopyIntoTraceBuffer();

  // The only thing that can be read right now is the 1st packet of the 2nd
  // sequence. All the rest is blocked waiting for patching.
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(16, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());

  // Now patch the 2nd sequence and check that the sequence is unblocked.
  ASSERT_TRUE(TryPatchChunkContents(ProducerID(2), WriterID(1), ChunkID(1),
                                    {{1, {{'P', 'A', 'T', 'C'}}}}));
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(),
              ElementsAre(FakePacketFragment("PATCd01-d02-d03", 15)));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(16, 'e')));
  ASSERT_THAT(ReadPacket(), IsEmpty());

  // Now patch the 3rd sequence, but in the first patch set
  // |other_patches_pending| to true, so that the sequence is unblocked only
  // after the 2nd patch.
  ASSERT_TRUE(TryPatchChunkContents(ProducerID(3), WriterID(1), ChunkID(0),
                                    {{1, {{'P', 'E', 'R', 'F'}}}},
                                    /*other_patches_pending=*/true));
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), IsEmpty());

  ASSERT_TRUE(TryPatchChunkContents(ProducerID(3), WriterID(1), ChunkID(0),
                                    {{5, {{'E', 'T', 'T', 'O'}}}},
                                    /*other_patches_pending=*/false));
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(),
              ElementsAre(FakePacketFragment("PERFETTOf02-f03", 15)));
  ASSERT_THAT(ReadPacket(), IsEmpty());

}  // namespace perfetto

// ---------------------
// Malicious input tests
// ---------------------

TEST_F(TraceBufferTest, Malicious_ZeroSizedChunk) {
  ResetBuffer(4096);
  SuppressSanityDchecksForTesting();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(32, 'a')
      .CopyIntoTraceBuffer();

  uint8_t valid_ptr = 0;
  trace_buffer()->CopyChunkUntrusted(
      ProducerID(1), uid_t(0), WriterID(1), ChunkID(1), 1 /* num packets */,
      0 /* flags*/, &valid_ptr, sizeof(valid_ptr));

  CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
      .AddPacket(32, 'b')
      .CopyIntoTraceBuffer();

  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(32, 'a')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(32, 'b')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Attempting to write a chunk bigger than ChunkRecord::kMaxSize should end up
// in a no-op.
TEST_F(TraceBufferTest, Malicious_ChunkTooBig) {
  ResetBuffer(4096);
  SuppressSanityDchecksForTesting();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(4096, 'a')
      .AddPacket(2048, 'b')
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Malicious_RepeatedChunkID) {
  ResetBuffer(4096);
  SuppressSanityDchecksForTesting();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(2048, 'a')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(1024, 'b')
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(1024, 'b')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Malicious_ZeroVarintHeader) {
  ResetBuffer(4096);
  SuppressSanityDchecksForTesting();
  // Create a standalone chunk where the varint header is == 0.
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(4, 'a')
      .ClearBytes(0, 1)
      .AddPacket(4, 'b')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(0))
      .AddPacket(4, 'c')
      .CopyIntoTraceBuffer();
  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4, 'c')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Malicious_VarintHeaderTooBig) {
  ResetBuffer(4096);
  SuppressSanityDchecksForTesting();

  // Add a valid chunk.
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(32, 'a')
      .CopyIntoTraceBuffer();

  // Forge a packet which has a varint header that is just off by one.
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(0))
      .AddPacket({0x16, '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
                  'c', 'd', 'e', 'f'})
      .CopyIntoTraceBuffer();

  // Forge a packet which has a varint header that tries to hit an overflow.
  CreateChunk(ProducerID(3), WriterID(1), ChunkID(0))
      .AddPacket({0xff, 0xff, 0xff, 0x7f})
      .CopyIntoTraceBuffer();

  // Forge a packet which has a jumbo varint header: 0xff, 0xff .. 0x7f.
  std::vector<uint8_t> chunk;
  chunk.insert(chunk.end(), 128 - sizeof(ChunkRecord), 0xff);
  chunk.back() = 0x7f;
  trace_buffer()->CopyChunkUntrusted(ProducerID(4), uid_t(0), WriterID(1),
                                     ChunkID(1), 1 /* num packets */,
                                     0 /* flags*/, chunk.data(), chunk.size());

  // Add a valid chunk.
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(32, 'b')
      .CopyIntoTraceBuffer();

  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(32, 'a')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(32, 'b')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Similar to Malicious_VarintHeaderTooBig, but this time the full chunk
// contains an enormous varint number that tries to overflow.
TEST_F(TraceBufferTest, Malicious_JumboVarint) {
  ResetBuffer(64 * 1024);
  SuppressSanityDchecksForTesting();

  std::vector<uint8_t> chunk;
  chunk.insert(chunk.end(), 64 * 1024 - sizeof(ChunkRecord) * 2, 0xff);
  chunk.back() = 0x7f;
  for (int i = 0; i < 3; i++) {
    trace_buffer()->CopyChunkUntrusted(
        ProducerID(1), uid_t(0), WriterID(1), ChunkID(1), 1 /* num packets */,
        0 /* flags*/, chunk.data(), chunk.size());
  }

  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Like the Malicious_ZeroVarintHeader, but put the chunk in the middle of a
// sequence that would be otherwise valid.
TEST_F(TraceBufferTest, Malicious_ZeroVarintHeaderInSequence) {
  ResetBuffer(4096);
  SuppressSanityDchecksForTesting();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(4, 'a', kContOnNextChunk)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(4, 'b', kContFromPrevChunk | kContOnNextChunk)
      .ClearBytes(0, 1)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(2))
      .AddPacket(4, 'c', kContFromPrevChunk)
      .AddPacket(4, 'd')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
      .AddPacket(4, 'e')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(3))
      .AddPacket(5, 'f')
      .CopyIntoTraceBuffer();

  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4, 'd')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4, 'e')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(5, 'f')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

// Similar to Malicious_ZeroVarintHeader, but this time the zero-sized fragment
// is the last fragment for a chunk and is marked for continuation.
// One might argue that this case is borderline legit, and in this case we
// should just read a packet consisting of (4, 'c'). However this is too complex
// to support and doesn't bring any benefit.
TEST_F(TraceBufferTest, Malicious_ZeroVarintHeaderAtEndOfChunk) {
  ResetBuffer(4096);
  SuppressSanityDchecksForTesting();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(4, 'a')
      .AddPacket(4, 'b', kContOnNextChunk)
      .ClearBytes(4, 4)
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(4, 'c', kContFromPrevChunk)
      .AddPacket(4, 'd')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(3))
      .AddPacket(4, 'e')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(2), WriterID(1), ChunkID(3))
      .AddPacket(4, 'f')
      .CopyIntoTraceBuffer();

  trace_buffer()->BeginRead();
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4, 'a')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4, 'd')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4, 'e')));
  ASSERT_THAT(ReadPacket(), ElementsAre(FakePacketFragment(4, 'f')));
  ASSERT_THAT(ReadPacket(), IsEmpty());
}

TEST_F(TraceBufferTest, Malicious_PatchOutOfBounds) {
  ResetBuffer(4096);
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(0))
      .AddPacket(2048, 'a')
      .CopyIntoTraceBuffer();
  CreateChunk(ProducerID(1), WriterID(1), ChunkID(1))
      .AddPacket(16, 'b')
      .CopyIntoTraceBuffer();
  size_t offsets[] = {13,          16,          size_t(-4),
                      size_t(-8),  size_t(-12), size_t(-16),
                      size_t(-20), size_t(-32), size_t(-1024)};
  for (size_t offset : offsets) {
    ASSERT_FALSE(TryPatchChunkContents(ProducerID(1), WriterID(1), ChunkID(1),
                                       {{offset, {{'0', 'd', 'a', 'y'}}}}));
  }
}

// -------------------
// SequenceIterator tests
// -------------------
TEST_F(TraceBufferTest, Iterator_OneStreamOrdered) {
  ResetBuffer(64 * 1024);
  AppendChunks({
      {ProducerID(1), WriterID(1), ChunkID(0)},
      {ProducerID(1), WriterID(1), ChunkID(1)},
      {ProducerID(1), WriterID(1), ChunkID(2)},
      {ProducerID(1), WriterID(1), ChunkID(5)},
      {ProducerID(1), WriterID(1), ChunkID(6)},
      {ProducerID(1), WriterID(1), ChunkID(7)},
  });
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(2), {}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(-1), WriterID(-1), {}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(1), {0, 1, 2, 5, 6, 7}));
}

TEST_F(TraceBufferTest, Iterator_OneStreamWrapping) {
  ResetBuffer(64 * 1024);
  AppendChunks({
      {ProducerID(1), WriterID(1), ChunkID(5)},
      {ProducerID(1), WriterID(1), ChunkID(6)},
      {ProducerID(1), WriterID(1), ChunkID(7)},
      {ProducerID(1), WriterID(1), ChunkID(0)},
      {ProducerID(1), WriterID(1), ChunkID(1)},
      {ProducerID(1), WriterID(1), ChunkID(2)},
  });
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(2), {}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(-1), WriterID(-1), {}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(1), {5, 6, 7, 0, 1, 2}));
}

TEST_F(TraceBufferTest, Iterator_ManyStreamsOrdered) {
  ResetBuffer(64 * 1024);
  AppendChunks({
      {ProducerID(1), WriterID(1), ChunkID(0)},
      {ProducerID(1), WriterID(1), ChunkID(1)},
      {ProducerID(1), WriterID(2), ChunkID(0)},
      {ProducerID(3), WriterID(1), ChunkID(0)},
      {ProducerID(1), WriterID(2), ChunkID(3)},
      {ProducerID(1), WriterID(2), ChunkID(5)},
      {ProducerID(3), WriterID(1), ChunkID(7)},
      {ProducerID(1), WriterID(1), ChunkID(6)},
      {ProducerID(3), WriterID(1), ChunkID(8)},
  });
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(1), {0, 1, 6}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(2), {0, 3, 5}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(3), WriterID(1), {0, 7, 8}));
}

TEST_F(TraceBufferTest, Iterator_ManyStreamsWrapping) {
  ResetBuffer(64 * 1024);
  auto Neg = [](int x) { return kMaxChunkID + x; };
  AppendChunks({
      {ProducerID(1), WriterID(1), ChunkID(Neg(-4))},
      {ProducerID(1), WriterID(1), ChunkID(Neg(-3))},
      {ProducerID(1), WriterID(2), ChunkID(Neg(-2))},
      {ProducerID(3), WriterID(1), ChunkID(Neg(-1))},
      {ProducerID(1), WriterID(2), ChunkID(0)},
      {ProducerID(1), WriterID(2), ChunkID(1)},
      {ProducerID(3), WriterID(1), ChunkID(2)},
      {ProducerID(1), WriterID(1), ChunkID(3)},
      {ProducerID(3), WriterID(1), ChunkID(4)},
  });
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(1), {Neg(-4), Neg(-3), 3}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(1), WriterID(2), {Neg(-2), 0, 1}));
  ASSERT_TRUE(IteratorSeqEq(ProducerID(3), WriterID(1), {Neg(-1), 2, 4}));
}

// TODO(primiano): test stats().
// TODO(primiano): test multiple streams interleaved.
// TODO(primiano): more testing on packet merging.

}  // namespace perfetto
