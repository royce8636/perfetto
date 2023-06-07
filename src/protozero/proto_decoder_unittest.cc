/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "perfetto/protozero/proto_decoder.h"

#include "perfetto/ext/base/utils.h"
#include "perfetto/protozero/message.h"
#include "perfetto/protozero/proto_utils.h"
#include "perfetto/protozero/scattered_heap_buffer.h"
#include "perfetto/protozero/static_buffer.h"
#include "test/gtest_and_gmock.h"

#include "src/protozero/test/example_proto/test_messages.pb.h"
#include "src/protozero/test/example_proto/test_messages.pbzero.h"

// Generated by the protozero plugin.
namespace pbtest = protozero::test::protos::pbzero;

// Generated by the official protobuf compiler.
namespace pbgold = protozero::test::protos;

namespace protozero {
namespace {

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using namespace proto_utils;

TEST(ProtoDecoderTest, ReadString) {
  HeapBuffered<Message> message;

  static constexpr char kTestString[] = "test";
  message->AppendString(1, kTestString);
  std::vector<uint8_t> proto = message.SerializeAsArray();
  TypedProtoDecoder<32, false> decoder(proto.data(), proto.size());

  const auto& field = decoder.Get(1);
  ASSERT_EQ(field.type(), ProtoWireType::kLengthDelimited);
  ASSERT_EQ(field.size(), sizeof(kTestString) - 1);
  for (size_t i = 0; i < sizeof(kTestString) - 1; i++) {
    ASSERT_EQ(field.data()[i], kTestString[i]);
  }
}

TEST(ProtoDecoderTest, SkipVeryLargeFields) {
  const size_t kPayloadSize = 257 * 1024 * 1024;
  const uint64_t data_size = 4096 + kPayloadSize;
  std::unique_ptr<uint8_t, perfetto::base::FreeDeleter> data(
      static_cast<uint8_t*>(malloc(data_size)));
  StaticBuffered<Message> message(data.get(), data_size);

  // Append a valid field.
  message->AppendVarInt(/*field_id=*/1, 11);

  // Append a very large field that will be skipped.
  uint8_t raw[10];
  uint8_t* wptr = raw;
  wptr = WriteVarInt(MakeTagLengthDelimited(2), wptr);
  wptr = WriteVarInt(kPayloadSize, wptr);
  message->AppendRawProtoBytes(raw, static_cast<size_t>(wptr - raw));
  const size_t kPaddingSize = 1024 * 128;
  std::unique_ptr<uint8_t[]> padding(new uint8_t[kPaddingSize]());
  for (size_t i = 0; i < kPayloadSize / kPaddingSize; i++)
    message->AppendRawProtoBytes(padding.get(), kPaddingSize);

  // Append another valid field.
  message->AppendVarInt(/*field_id=*/3, 13);

  ProtoDecoder decoder(data.get(), message.Finalize());
  Field field = decoder.ReadField();
  ASSERT_EQ(1u, field.id());
  ASSERT_EQ(11, field.as_int32());

  field = decoder.ReadField();
  ASSERT_EQ(3u, field.id());
  ASSERT_EQ(13, field.as_int32());

  field = decoder.ReadField();
  ASSERT_FALSE(field.valid());
}

TEST(ProtoDecoderTest, SingleRepeatedField) {
  HeapBuffered<Message> message;
  message->AppendVarInt(/*field_id=*/2, 10);
  auto data = message.SerializeAsArray();
  TypedProtoDecoder<2, true> tpd(data.data(), data.size());
  auto it = tpd.GetRepeated<int32_t>(/*field_id=*/2);
  EXPECT_TRUE(it);
  EXPECT_EQ(it.field().as_int32(), 10);
  EXPECT_EQ(*it, 10);
  EXPECT_FALSE(++it);
}

TEST(ProtoDecoderTest, RepeatedVariableLengthField) {
  HeapBuffered<Message> message;

  static constexpr char kTestString[] = "test";
  static constexpr char kTestString2[] = "honk honk";
  message->AppendString(1, kTestString);
  message->AppendString(1, kTestString2);
  std::vector<uint8_t> proto = message.SerializeAsArray();
  TypedProtoDecoder<32, false> decoder(proto.data(), proto.size());

  auto it = decoder.GetRepeated<ConstChars>(1);
  ASSERT_EQ(it->type(), ProtoWireType::kLengthDelimited);
  ASSERT_EQ(it->size(), sizeof(kTestString) - 1);
  ASSERT_EQ(it->as_std_string(), std::string(kTestString));
  ASSERT_EQ((*it).ToStdString(), std::string(kTestString));
  ++it;
  ASSERT_EQ(it->type(), ProtoWireType::kLengthDelimited);
  ASSERT_EQ(it->size(), sizeof(kTestString2) - 1);
  ASSERT_EQ(it->as_std_string(), std::string(kTestString2));
  ASSERT_EQ((*it).ToStdString(), std::string(kTestString2));
}

TEST(ProtoDecoderTest, SingleRepeatedFieldWithExpansion) {
  HeapBuffered<Message> message;
  for (int i = 0; i < 2000; i++) {
    message->AppendVarInt(/*field_id=*/2, i);
  }
  auto data = message.SerializeAsArray();
  TypedProtoDecoder<2, true> tpd(data.data(), data.size());
  auto it = tpd.GetRepeated<int32_t>(/*field_id=*/2);
  for (int i = 0; i < 2000; i++) {
    EXPECT_TRUE(it);
    EXPECT_EQ(*it, i);
    ++it;
  }
  EXPECT_FALSE(it);
}

TEST(ProtoDecoderTest, NoRepeatedField) {
  uint8_t buf[] = {0x01};
  TypedProtoDecoder<2, true> tpd(buf, 1);
  auto it = tpd.GetRepeated<int32_t>(/*field_id=*/1);
  EXPECT_FALSE(it);
  EXPECT_FALSE(tpd.Get(2).valid());
}

TEST(ProtoDecoderTest, RepeatedFields) {
  HeapBuffered<Message> message;

  message->AppendVarInt(1, 10);
  message->AppendVarInt(2, 20);
  message->AppendVarInt(3, 30);

  message->AppendVarInt(1, 11);
  message->AppendVarInt(2, 21);
  message->AppendVarInt(2, 22);

  // When iterating with the simple decoder we should just see fields in parsing
  // order.
  auto data = message.SerializeAsArray();
  ProtoDecoder decoder(data.data(), data.size());
  std::string fields_seen;
  for (auto fld = decoder.ReadField(); fld.valid(); fld = decoder.ReadField()) {
    fields_seen +=
        std::to_string(fld.id()) + ":" + std::to_string(fld.as_int32()) + ";";
  }
  EXPECT_EQ(fields_seen, "1:10;2:20;3:30;1:11;2:21;2:22;");

  TypedProtoDecoder<4, true> tpd(data.data(), data.size());

  // When parsing with the one-shot decoder and querying the single field id, we
  // should see the last value for each of them, not the first one. This is the
  // current behavior of Google protobuf's parser.
  EXPECT_EQ(tpd.Get(1).as_int32(), 11);
  EXPECT_EQ(tpd.Get(2).as_int32(), 22);
  EXPECT_EQ(tpd.Get(3).as_int32(), 30);

  // But when iterating we should see values in the original order.
  auto it = tpd.GetRepeated<int32_t>(1);
  EXPECT_EQ(*it, 10);
  EXPECT_EQ(*++it, 11);
  EXPECT_FALSE(++it);

  it = tpd.GetRepeated<int32_t>(2);
  EXPECT_EQ(*it++, 20);
  EXPECT_EQ(*it++, 21);
  EXPECT_EQ(*it++, 22);
  EXPECT_FALSE(it);

  it = tpd.GetRepeated<int32_t>(3);
  EXPECT_EQ(*it, 30);
  EXPECT_FALSE(++it);
}

TEST(ProtoDecoderTest, FixedData) {
  struct FieldExpectation {
    const char* encoded;
    size_t encoded_size;
    uint32_t id;
    ProtoWireType type;
    uint64_t int_value;
  };

  const FieldExpectation kFieldExpectations[] = {
      {"\x08\x00", 2, 1, ProtoWireType::kVarInt, 0},
      {"\x08\x01", 2, 1, ProtoWireType::kVarInt, 1},
      {"\x08\x42", 2, 1, ProtoWireType::kVarInt, 0x42},
      {"\xF8\x07\x42", 3, 127, ProtoWireType::kVarInt, 0x42},
      {"\xB8\x3E\xFF\xFF\xFF\xFF\x0F", 7, 999, ProtoWireType::kVarInt,
       0xFFFFFFFF},
      {"\x7D\x42\x00\x00\x00", 5, 15, ProtoWireType::kFixed32, 0x42},
      {"\xBD\x3E\x78\x56\x34\x12", 6, 999, ProtoWireType::kFixed32, 0x12345678},
      {"\x79\x42\x00\x00\x00\x00\x00\x00\x00", 9, 15, ProtoWireType::kFixed64,
       0x42},
      {"\xB9\x3E\x08\x07\x06\x05\x04\x03\x02\x01", 10, 999,
       ProtoWireType::kFixed64, 0x0102030405060708},
      {"\x0A\x00", 2, 1, ProtoWireType::kLengthDelimited, 0},
      {"\x0A\x04|abc", 6, 1, ProtoWireType::kLengthDelimited, 4},
      {"\xBA\x3E\x04|abc", 7, 999, ProtoWireType::kLengthDelimited, 4},
      {"\xBA\x3E\x83\x01|abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzab"
       "cdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstu"
       "vwx",
       135, 999, ProtoWireType::kLengthDelimited, 131},
  };

  for (size_t i = 0; i < perfetto::base::ArraySize(kFieldExpectations); ++i) {
    const FieldExpectation& exp = kFieldExpectations[i];
    TypedProtoDecoder<999, 0> decoder(
        reinterpret_cast<const uint8_t*>(exp.encoded), exp.encoded_size);

    auto& field = decoder.Get(exp.id);
    ASSERT_EQ(exp.type, field.type());

    if (field.type() == ProtoWireType::kLengthDelimited) {
      ASSERT_EQ(exp.int_value, field.size());
    } else {
      ASSERT_EQ(int64_t(exp.int_value), field.as_int64());
      // Proto encodes booleans as varints of 0 or 1.
      if (exp.int_value == 0 || exp.int_value == 1) {
        ASSERT_EQ(int64_t(exp.int_value), field.as_bool());
      }
    }
  }

  // Test float and doubles decoding.
  const char buf[] = "\x0d\x00\x00\xa0\x3f\x11\x00\x00\x00\x00\x00\x42\x8f\xc0";
  TypedProtoDecoder<2, false> decoder(reinterpret_cast<const uint8_t*>(buf),
                                      sizeof(buf));
  EXPECT_FLOAT_EQ(decoder.Get(1).as_float(), 1.25f);
  EXPECT_DOUBLE_EQ(decoder.Get(2).as_double(), -1000.25);
}

TEST(ProtoDecoderTest, FindField) {
  uint8_t buf[] = {0x08, 0x00};  // field_id 1, varint value 0.
  ProtoDecoder pd(buf, 2);

  auto field = pd.FindField(1);
  ASSERT_TRUE(field);
  EXPECT_EQ(field.as_int64(), 0);

  auto field2 = pd.FindField(2);
  EXPECT_FALSE(field2);
}

TEST(ProtoDecoderTest, MoveTypedDecoder) {
  HeapBuffered<Message> message;
  message->AppendVarInt(/*field_id=*/1, 10);
  std::vector<uint8_t> proto = message.SerializeAsArray();

  // Construct a decoder that uses inline storage (i.e., the fields are stored
  // within the object itself).
  using Decoder = TypedProtoDecoder<32, false>;
  std::unique_ptr<Decoder> decoder(new Decoder(proto.data(), proto.size()));
  ASSERT_GE(reinterpret_cast<uintptr_t>(&decoder->at<1>()),
            reinterpret_cast<uintptr_t>(decoder.get()));
  ASSERT_LT(reinterpret_cast<uintptr_t>(&decoder->at<1>()),
            reinterpret_cast<uintptr_t>(decoder.get()) + sizeof(Decoder));

  // Move the decoder into another object and deallocate the original object.
  Decoder decoder2(std::move(*decoder));
  decoder.reset();

  // Check that the contents got moved correctly.
  EXPECT_EQ(decoder2.Get(1).as_int32(), 10);
  ASSERT_GE(reinterpret_cast<uintptr_t>(&decoder2.at<1>()),
            reinterpret_cast<uintptr_t>(&decoder2));
  ASSERT_LT(reinterpret_cast<uintptr_t>(&decoder2.at<1>()),
            reinterpret_cast<uintptr_t>(&decoder2) + sizeof(Decoder));
}

TEST(ProtoDecoderTest, PackedRepeatedVarint) {
  std::vector<int32_t> values = {42, 255, 0, -1};

  // serialize using protobuf library
  pbgold::PackedRepeatedFields msg;
  for (auto v : values)
    msg.add_field_int32(v);
  std::string serialized = msg.SerializeAsString();

  // decode using TypedProtoDecoder directly
  {
    constexpr int kFieldId =
        pbtest::PackedRepeatedFields::kFieldInt32FieldNumber;
    TypedProtoDecoder<kFieldId, false> decoder(
        reinterpret_cast<const uint8_t*>(serialized.data()), serialized.size());
    ASSERT_TRUE(decoder.at<kFieldId>().valid());
    bool parse_error = false;
    auto packed_it =
        decoder.GetPackedRepeated<proto_utils::ProtoWireType::kVarInt, int32_t>(
            kFieldId, &parse_error);

    std::vector<int32_t> decoded_values;
    for (; packed_it; ++packed_it) {
      auto v = *packed_it;
      decoded_values.push_back(v);
    }
    ASSERT_EQ(values, decoded_values);
    ASSERT_FALSE(parse_error);
  }

  // decode using plugin-generated accessor
  {
    auto decoder = pbtest::PackedRepeatedFields::Decoder(serialized);
    ASSERT_TRUE(decoder.has_field_int32());

    bool parse_error = false;
    std::vector<int32_t> decoded_values;
    for (auto packed_it = decoder.field_int32(&parse_error); packed_it;
         ++packed_it) {
      auto v = *packed_it;
      decoded_values.push_back(v);
    }
    ASSERT_EQ(values, decoded_values);
    ASSERT_FALSE(parse_error);
  }

  // unset field case
  pbgold::PackedRepeatedFields empty_msg;
  std::string empty_serialized = empty_msg.SerializeAsString();
  auto decoder = pbtest::PackedRepeatedFields::Decoder(empty_serialized);
  ASSERT_FALSE(decoder.has_field_int32());
  bool parse_error = false;
  auto packed_it = decoder.field_int32(&parse_error);
  ASSERT_FALSE(bool(packed_it));
  ASSERT_FALSE(parse_error);
}

TEST(ProtoDecoderTest, PackedRepeatedFixed32) {
  std::vector<uint32_t> values = {42, 255, 0, 1};

  // serialize using protobuf library
  pbgold::PackedRepeatedFields msg;
  for (auto v : values)
    msg.add_field_fixed32(v);
  std::string serialized = msg.SerializeAsString();

  // decode using TypedProtoDecoder directly
  {
    constexpr int kFieldId =
        pbtest::PackedRepeatedFields::kFieldFixed32FieldNumber;
    TypedProtoDecoder<kFieldId, false> decoder(
        reinterpret_cast<const uint8_t*>(serialized.data()), serialized.size());
    bool parse_error = false;
    auto packed_it =
        decoder
            .GetPackedRepeated<proto_utils::ProtoWireType::kFixed32, uint32_t>(
                kFieldId, &parse_error);

    std::vector<uint32_t> decoded_values;
    for (; packed_it; ++packed_it) {
      auto v = *packed_it;
      decoded_values.push_back(v);
    }
    ASSERT_EQ(values, decoded_values);
    ASSERT_FALSE(parse_error);
  }

  // decode using plugin-generated accessor
  {
    auto decoder = pbtest::PackedRepeatedFields::Decoder(serialized);
    ASSERT_TRUE(decoder.has_field_fixed32());

    bool parse_error = false;
    std::vector<uint32_t> decoded_values;
    for (auto packed_it = decoder.field_fixed32(&parse_error); packed_it;
         packed_it++) {
      auto v = *packed_it;
      decoded_values.push_back(v);
    }
    ASSERT_EQ(values, decoded_values);
    ASSERT_FALSE(parse_error);
  }

  // unset field case
  pbgold::PackedRepeatedFields empty_msg;
  std::string empty_serialized = empty_msg.SerializeAsString();
  auto decoder = pbtest::PackedRepeatedFields::Decoder(empty_serialized);
  ASSERT_FALSE(decoder.has_field_fixed32());
  bool parse_error = false;
  auto packed_it = decoder.field_fixed32(&parse_error);
  ASSERT_FALSE(bool(packed_it));
  ASSERT_FALSE(parse_error);
}

TEST(ProtoDecoderTest, PackedRepeatedFixed64) {
  std::vector<int64_t> values = {42, 255, 0, -1};

  // serialize using protobuf library
  pbgold::PackedRepeatedFields msg;
  for (auto v : values)
    msg.add_field_sfixed64(v);
  std::string serialized = msg.SerializeAsString();

  // decode using TypedProtoDecoder directly
  {
    constexpr int kFieldId =
        pbtest::PackedRepeatedFields::kFieldSfixed64FieldNumber;
    TypedProtoDecoder<kFieldId, false> decoder(
        reinterpret_cast<const uint8_t*>(serialized.data()), serialized.size());
    bool parse_error = false;
    auto packed_it =
        decoder
            .GetPackedRepeated<proto_utils::ProtoWireType::kFixed64, int64_t>(
                kFieldId, &parse_error);

    std::vector<int64_t> decoded_values;
    for (; packed_it; ++packed_it) {
      auto v = *packed_it;
      decoded_values.push_back(v);
    }
    ASSERT_EQ(values, decoded_values);
    ASSERT_FALSE(parse_error);
  }

  // decode using plugin-generated accessor
  {
    auto decoder = pbtest::PackedRepeatedFields::Decoder(serialized);
    ASSERT_TRUE(decoder.has_field_sfixed64());

    bool parse_error = false;
    std::vector<int64_t> decoded_values;
    for (auto packed_it = decoder.field_sfixed64(&parse_error); packed_it;
         packed_it++) {
      auto v = *packed_it;
      decoded_values.push_back(v);
    }
    ASSERT_EQ(values, decoded_values);
    ASSERT_FALSE(parse_error);
  }

  // unset field case
  pbgold::PackedRepeatedFields empty_msg;
  std::string empty_serialized = empty_msg.SerializeAsString();
  auto decoder = pbtest::PackedRepeatedFields::Decoder(empty_serialized);
  ASSERT_FALSE(decoder.has_field_sfixed64());
  bool parse_error = false;
  auto packed_it = decoder.field_sfixed64(&parse_error);
  ASSERT_FALSE(bool(packed_it));
  ASSERT_FALSE(parse_error);
}

TEST(ProtoDecoderTest, ZeroLengthPackedRepeatedField) {
  HeapBuffered<pbtest::PackedRepeatedFields> msg;
  PackedVarInt buf;
  msg->set_field_int32(buf);
  std::string serialized = msg.SerializeAsString();

  // Encoded as 2 bytes: tag/field, and a length of zero.
  EXPECT_EQ(2u, serialized.size());

  // Appears empty when decoded.
  auto decoder = pbtest::PackedRepeatedFields::Decoder(serialized);
  ASSERT_TRUE(decoder.has_field_int32());
  bool parse_error = false;
  auto packed_it = decoder.field_int32(&parse_error);
  ASSERT_FALSE(bool(packed_it));
  ASSERT_FALSE(parse_error);
}

TEST(ProtoDecoderTest, MalformedPackedFixedBuffer) {
  // Encode a fixed32 field where the length is not a multiple of 4 bytes.
  HeapBuffered<pbtest::PackedRepeatedFields> msg;
  PackedFixedSizeInt<uint32_t> buf;
  buf.Append(1);
  buf.Append(2);
  buf.Append(3);
  const uint8_t* data = buf.data();
  size_t size = buf.size();
  size_t invalid_size = size - 2;
  constexpr int kFieldId =
      pbtest::PackedRepeatedFields::kFieldFixed32FieldNumber;
  msg->AppendBytes(kFieldId, data, invalid_size);
  std::string serialized = msg.SerializeAsString();

  // Iterator indicates parse error.
  auto decoder = pbtest::PackedRepeatedFields::Decoder(serialized);
  ASSERT_TRUE(decoder.has_field_fixed32());
  bool parse_error = false;
  for (auto packed_it = decoder.field_fixed32(&parse_error); packed_it;
       packed_it++) {
  }
  ASSERT_TRUE(parse_error);
}

TEST(ProtoDecoderTest, MalformedPackedVarIntBuffer) {
  // Encode a varint field with the last varint chopped off partway.
  HeapBuffered<pbtest::PackedRepeatedFields> msg;
  PackedVarInt buf;
  buf.Append(1024);
  buf.Append(2048);
  buf.Append(4096);
  const uint8_t* data = buf.data();
  size_t size = buf.size();
  size_t invalid_size = size - 1;
  constexpr int kFieldId = pbtest::PackedRepeatedFields::kFieldInt32FieldNumber;
  msg->AppendBytes(kFieldId, data, invalid_size);
  std::string serialized = msg.SerializeAsString();

  // Iterator indicates parse error.
  auto decoder = pbtest::PackedRepeatedFields::Decoder(serialized);
  ASSERT_TRUE(decoder.has_field_int32());
  bool parse_error = false;
  for (auto packed_it = decoder.field_int32(&parse_error); packed_it;
       packed_it++) {
  }
  ASSERT_TRUE(parse_error);
}

// Tests that:
// 1. Very big field ids (>= 2**24) are just skipped but don't fail parsing.
//    This is a regression test for b/145339282 (DataSourceConfig.for_testing
//    having a very large ID == 268435455 until Android R).
// 2. Moderately big" field ids can be parsed correctly. See also
//    https://github.com/google/perfetto/issues/510 .
TEST(ProtoDecoderTest, BigFieldIds) {
  HeapBuffered<Message> message;
  message->AppendVarInt(/*field_id=*/1, 11);
  message->AppendVarInt(/*field_id=*/1 << 24, 0);  // Will be skipped
  message->AppendVarInt(/*field_id=*/65535, 99);
  message->AppendVarInt(/*field_id=*/(1 << 24) + 1023,
                        0);  // Will be skipped
  message->AppendVarInt(/*field_id=*/2, 12);
  message->AppendVarInt(/*field_id=*/1 << 28, 0);  // Will be skipped

  message->AppendVarInt(/*field_id=*/(1 << 24) - 1, 13);
  auto data = message.SerializeAsArray();

  // Check the iterator-based ProtoDecoder.
  {
    ProtoDecoder decoder(data.data(), data.size());
    Field field = decoder.ReadField();
    ASSERT_TRUE(field.valid());
    ASSERT_EQ(field.id(), 1u);
    ASSERT_EQ(field.as_int32(), 11);

    field = decoder.ReadField();
    ASSERT_TRUE(field.valid());
    ASSERT_EQ(field.id(), 65535u);
    ASSERT_EQ(field.as_int32(), 99);

    field = decoder.ReadField();
    ASSERT_TRUE(field.valid());
    ASSERT_EQ(field.id(), 2u);
    ASSERT_EQ(field.as_int32(), 12);

    field = decoder.ReadField();
    ASSERT_TRUE(field.valid());
    ASSERT_EQ(field.id(), (1u << 24) - 1u);
    ASSERT_EQ(field.as_int32(), 13);

    field = decoder.ReadField();
    ASSERT_FALSE(field.valid());
  }

  // Test the one-shot-read TypedProtoDecoder.
  // Note: field 65535 will be also skipped because this TypedProtoDecoder has
  // a cap on MAX_FIELD_ID = 3.
  {
    TypedProtoDecoder<3, true> tpd(data.data(), data.size());
    EXPECT_EQ(tpd.Get(1).as_int32(), 11);
    EXPECT_EQ(tpd.Get(2).as_int32(), 12);
  }
}

// Edge case for SkipBigFieldIds, the message contains only one field with a
// very big id. Test that we skip it and return an invalid field, instead of
// geetting stuck in some loop.
TEST(ProtoDecoderTest, OneBigFieldIdOnly) {
  HeapBuffered<Message> message;
  message->AppendVarInt(/*field_id=*/268435455, 0);
  auto data = message.SerializeAsArray();

  // Check the iterator-based ProtoDecoder.
  ProtoDecoder decoder(data.data(), data.size());
  Field field = decoder.ReadField();
  ASSERT_FALSE(field.valid());
}

// Check what happens when trying to parse packed repeated field and finding a
// mismatching wire type instead. A compliant protobuf decoder should accept it,
// but protozero doesn't handle that. At least it shouldn't crash.
TEST(ProtoDecoderTest, PacketRepeatedWireTypeMismatch) {
  protozero::HeapBuffered<pbtest::PackedRepeatedFields> message;
  // A proper packed encoding should have a length delimited wire type. Use a
  // var int wire type instead.
  constexpr int kFieldId = pbtest::PackedRepeatedFields::kFieldInt32FieldNumber;
  message->AppendTinyVarInt(kFieldId, 5);
  auto data = message.SerializeAsArray();

  pbtest::PackedRepeatedFields::Decoder decoder(data.data(), data.size());
  bool parse_error = false;
  auto it = decoder.field_int32(&parse_error);
  // The decoder doesn't return a parse error (maybe it should, but that has
  // been the behavior since the beginning).
  ASSERT_FALSE(parse_error);
  // But the iterator returns 0 elements.
  EXPECT_FALSE(it);
}

}  // namespace
}  // namespace protozero
