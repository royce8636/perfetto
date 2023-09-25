/*
 * Copyright (C) 2023 The Android Open Source Project
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

#ifndef SRC_SHARED_LIB_TEST_PROTOS_TEST_MESSAGES_PZC_H_
#define SRC_SHARED_LIB_TEST_PROTOS_TEST_MESSAGES_PZC_H_

#include <stdbool.h>
#include <stdint.h>

#include "perfetto/public/pb_macros.h"
#include "src/shared_lib/test/protos/library.pzc.h"

PERFETTO_PB_MSG_DECL(protozero_test_protos_EveryField);
PERFETTO_PB_MSG_DECL(protozero_test_protos_NestedA_NestedB);
PERFETTO_PB_MSG_DECL(protozero_test_protos_NestedA_NestedB_NestedC);
PERFETTO_PB_MSG_DECL(protozero_test_protos_TestVersioning_V1_Sub1_V1);
PERFETTO_PB_MSG_DECL(protozero_test_protos_TestVersioning_V2_Sub1_V2);
PERFETTO_PB_MSG_DECL(protozero_test_protos_TestVersioning_V2_Sub2_V2);
PERFETTO_PB_MSG_DECL(protozero_test_protos_TransgalacticMessage);

PERFETTO_PB_ENUM(protozero_test_protos_SmallEnum){
    PERFETTO_PB_ENUM_ENTRY(protozero_test_protos_TO_BE) = 1,
    PERFETTO_PB_ENUM_ENTRY(protozero_test_protos_NOT_TO_BE) = 0,
};

PERFETTO_PB_ENUM(protozero_test_protos_SignedEnum){
    PERFETTO_PB_ENUM_ENTRY(protozero_test_protos_POSITIVE) = 1,
    PERFETTO_PB_ENUM_ENTRY(protozero_test_protos_NEUTRAL) = 0,
    PERFETTO_PB_ENUM_ENTRY(protozero_test_protos_NEGATIVE) = -1,
};

PERFETTO_PB_ENUM(protozero_test_protos_BigEnum){
    PERFETTO_PB_ENUM_ENTRY(protozero_test_protos_BEGIN) = 10,
    PERFETTO_PB_ENUM_ENTRY(protozero_test_protos_END) = 100500,
};

PERFETTO_PB_ENUM_IN_MSG(protozero_test_protos_TestVersioning_V2, Enumz_V2){
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(protozero_test_protos_TestVersioning_V2,
                                  ONE) = 1,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(protozero_test_protos_TestVersioning_V2,
                                  TWO) = 2,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(protozero_test_protos_TestVersioning_V2,
                                  THREE_V2) = 3,
};

PERFETTO_PB_ENUM_IN_MSG(protozero_test_protos_TestVersioning_V1, Enumz_V1){
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(protozero_test_protos_TestVersioning_V1,
                                  ONE) = 1,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(protozero_test_protos_TestVersioning_V1,
                                  TWO) = 2,
};

PERFETTO_PB_ENUM_IN_MSG(protozero_test_protos_EveryField, NestedEnum){
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(protozero_test_protos_EveryField, PING) = 1,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(protozero_test_protos_EveryField, PONG) = 2,
};

PERFETTO_PB_MSG(protozero_test_protos_TestVersioning_V2);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  VARINT,
                  int32_t,
                  root_int,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  VARINT,
                  enum protozero_test_protos_TestVersioning_V2_Enumz_V2,
                  enumz,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  STRING,
                  const char*,
                  root_string,
                  3);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  STRING,
                  const char*,
                  rep_string,
                  4);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  MSG,
                  protozero_test_protos_TestVersioning_V2_Sub1_V2,
                  sub1,
                  5);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  MSG,
                  protozero_test_protos_TestVersioning_V2_Sub1_V2,
                  sub1_rep,
                  6);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  MSG,
                  protozero_test_protos_TestVersioning_V2_Sub1_V2,
                  sub1_lazy,
                  7);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  VARINT,
                  int32_t,
                  root_int_v2,
                  10);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  MSG,
                  protozero_test_protos_TestVersioning_V2_Sub2_V2,
                  sub2,
                  11);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  MSG,
                  protozero_test_protos_TestVersioning_V2_Sub2_V2,
                  sub2_rep,
                  12);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2,
                  MSG,
                  protozero_test_protos_TestVersioning_V2_Sub2_V2,
                  sub2_lazy,
                  13);

PERFETTO_PB_MSG(protozero_test_protos_TestVersioning_V2_Sub2_V2);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2_Sub2_V2,
                  VARINT,
                  int32_t,
                  sub2_int,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2_Sub2_V2,
                  STRING,
                  const char*,
                  sub2_string,
                  2);

PERFETTO_PB_MSG(protozero_test_protos_TestVersioning_V2_Sub1_V2);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2_Sub1_V2,
                  VARINT,
                  int32_t,
                  sub1_int,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2_Sub1_V2,
                  STRING,
                  const char*,
                  sub1_string,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2_Sub1_V2,
                  VARINT,
                  int32_t,
                  sub1_int_v2,
                  3);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V2_Sub1_V2,
                  STRING,
                  const char*,
                  sub1_string_v2,
                  4);

PERFETTO_PB_MSG(protozero_test_protos_TestVersioning_V1);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1,
                  VARINT,
                  int32_t,
                  root_int,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1,
                  VARINT,
                  enum protozero_test_protos_TestVersioning_V1_Enumz_V1,
                  enumz,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1,
                  STRING,
                  const char*,
                  root_string,
                  3);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1,
                  STRING,
                  const char*,
                  rep_string,
                  4);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1,
                  MSG,
                  protozero_test_protos_TestVersioning_V1_Sub1_V1,
                  sub1,
                  5);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1,
                  MSG,
                  protozero_test_protos_TestVersioning_V1_Sub1_V1,
                  sub1_rep,
                  6);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1,
                  MSG,
                  protozero_test_protos_TestVersioning_V1_Sub1_V1,
                  sub1_lazy,
                  7);

PERFETTO_PB_MSG(protozero_test_protos_TestVersioning_V1_Sub1_V1);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1_Sub1_V1,
                  VARINT,
                  int32_t,
                  sub1_int,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_TestVersioning_V1_Sub1_V1,
                  STRING,
                  const char*,
                  sub1_string,
                  2);

PERFETTO_PB_MSG(protozero_test_protos_PackedRepeatedFields);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Int32,
                  field_int32,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Int64,
                  field_int64,
                  4);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Uint32,
                  field_uint32,
                  5);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Uint64,
                  field_uint64,
                  6);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Fixed32,
                  field_fixed32,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Fixed64,
                  field_fixed64,
                  9);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Sfixed32,
                  field_sfixed32,
                  10);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Sfixed64,
                  field_sfixed64,
                  3);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Float,
                  field_float,
                  11);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Double,
                  field_double,
                  12);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Int32,
                  small_enum,
                  51);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Int32,
                  signed_enum,
                  52);
PERFETTO_PB_FIELD(protozero_test_protos_PackedRepeatedFields,
                  PACKED,
                  Int32,
                  big_enum,
                  53);

PERFETTO_PB_MSG(protozero_test_protos_CamelCaseFields);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields,
                  VARINT,
                  bool,
                  foo_bar_baz,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields,
                  VARINT,
                  bool,
                  barbaz,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields,
                  VARINT,
                  bool,
                  moomoo,
                  3);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields,
                  VARINT,
                  bool,
                  urlencoder,
                  4);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields, VARINT, bool, xmap, 5);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields,
                  VARINT,
                  bool,
                  urle_nco__der,
                  6);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields,
                  VARINT,
                  bool,
                  __bigbang,
                  7);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields, VARINT, bool, u2, 8);
PERFETTO_PB_FIELD(protozero_test_protos_CamelCaseFields,
                  VARINT,
                  bool,
                  bangbig__,
                  9);

PERFETTO_PB_MSG(protozero_test_protos_NestedA);
PERFETTO_PB_FIELD(protozero_test_protos_NestedA,
                  MSG,
                  protozero_test_protos_NestedA_NestedB,
                  repeated_a,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_NestedA,
                  MSG,
                  protozero_test_protos_NestedA_NestedB_NestedC,
                  super_nested,
                  3);

PERFETTO_PB_MSG(protozero_test_protos_NestedA_NestedB);
PERFETTO_PB_FIELD(protozero_test_protos_NestedA_NestedB,
                  MSG,
                  protozero_test_protos_NestedA_NestedB_NestedC,
                  value_b,
                  1);

PERFETTO_PB_MSG(protozero_test_protos_NestedA_NestedB_NestedC);
PERFETTO_PB_FIELD(protozero_test_protos_NestedA_NestedB_NestedC,
                  VARINT,
                  int32_t,
                  value_c,
                  1);

PERFETTO_PB_MSG(protozero_test_protos_EveryField);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  int32_t,
                  field_int32,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  int64_t,
                  field_int64,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  uint32_t,
                  field_uint32,
                  3);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  uint64_t,
                  field_uint64,
                  4);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  ZIGZAG,
                  int32_t,
                  field_sint32,
                  5);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  ZIGZAG,
                  int64_t,
                  field_sint64,
                  6);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED32,
                  uint32_t,
                  field_fixed32,
                  7);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED64,
                  uint64_t,
                  field_fixed64,
                  8);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED32,
                  int32_t,
                  field_sfixed32,
                  9);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED64,
                  int64_t,
                  field_sfixed64,
                  10);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED32,
                  float,
                  field_float,
                  11);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED64,
                  double,
                  field_double,
                  12);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  bool,
                  field_bool,
                  13);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  MSG,
                  protozero_test_protos_EveryField,
                  field_nested,
                  14);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  enum protozero_test_protos_SmallEnum,
                  small_enum,
                  51);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  enum protozero_test_protos_SignedEnum,
                  signed_enum,
                  52);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  enum protozero_test_protos_BigEnum,
                  big_enum,
                  53);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  STRING,
                  const char*,
                  field_string,
                  500);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  STRING,
                  const char*,
                  field_bytes,
                  505);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  enum protozero_test_protos_EveryField_NestedEnum,
                  nested_enum,
                  600);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  STRING,
                  const char*,
                  repeated_string,
                  700);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED64,
                  uint64_t,
                  repeated_fixed64,
                  701);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  FIXED32,
                  int32_t,
                  repeated_sfixed32,
                  702);
PERFETTO_PB_FIELD(protozero_test_protos_EveryField,
                  VARINT,
                  int32_t,
                  repeated_int32,
                  999);

PERFETTO_PB_MSG(protozero_test_protos_TransgalacticParcel);
PERFETTO_PB_FIELD(protozero_test_protos_TransgalacticParcel,
                  MSG,
                  protozero_test_protos_TransgalacticMessage,
                  message,
                  1);
PERFETTO_PB_FIELD(protozero_test_protos_TransgalacticParcel,
                  STRING,
                  const char*,
                  tracking_code,
                  2);
PERFETTO_PB_FIELD(protozero_test_protos_TransgalacticParcel,
                  VARINT,
                  enum protozero_test_protos_TransgalacticMessage_MessageType,
                  message_type,
                  3);

#endif  // SRC_SHARED_LIB_TEST_PROTOS_TEST_MESSAGES_PZC_H_
