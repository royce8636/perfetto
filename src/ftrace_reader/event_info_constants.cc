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

#include "event_info_constants.h"

namespace perfetto {

Field MakeField(const char* name, size_t id, ProtoFieldType type) {
  Field field{};
  field.ftrace_name = name;
  field.proto_field_id = id;
  field.proto_field_type = type;
  return field;
}

std::vector<Field> GetStaticCommonFieldsInfo() {
  std::vector<Field> fields;

  fields.push_back(MakeField("common_pid", 2, kProtoInt32));

  return fields;
}

bool SetTranslationStrategy(FtraceFieldType ftrace,
                            ProtoFieldType proto,
                            TranslationStrategy* out) {
  if (ftrace == kFtraceUint8 && proto == kProtoUint32) {
    *out = kUint8ToUint32;
  } else if (ftrace == kFtraceUint16 && proto == kProtoUint32) {
    *out = kUint16ToUint32;
  } else if (ftrace == kFtraceUint32 && proto == kProtoUint32) {
    *out = kUint32ToUint32;
  } else if (ftrace == kFtraceUint32 && proto == kProtoUint64) {
    *out = kUint32ToUint64;
  } else if (ftrace == kFtraceUint64 && proto == kProtoUint64) {
    *out = kUint64ToUint64;
  } else if (ftrace == kFtraceInt16 && proto == kProtoInt32) {
    *out = kInt16ToInt32;
  } else if (ftrace == kFtraceInt32 && proto == kProtoInt32) {
    *out = kInt32ToInt32;
  } else if (ftrace == kFtraceInt32 && proto == kProtoInt64) {
    *out = kInt32ToInt64;
  } else if (ftrace == kFtraceInt64 && proto == kProtoInt64) {
    *out = kInt64ToInt64;
  } else if (ftrace == kFtraceFixedCString && proto == kProtoString) {
    *out = kFixedCStringToString;
  } else if (ftrace == kFtraceCString && proto == kProtoString) {
    *out = kCStringToString;
  } else if (ftrace == kFtraceStringPtr && proto == kProtoString) {
    *out = kStringPtrToString;
  } else if (ftrace == kFtraceBool && proto == kProtoUint32) {
    *out = kBoolToUint32;
  } else {
    PERFETTO_DLOG("No translation strategy for '%s' -> '%s'", ToString(ftrace),
                  ToString(proto));
    return false;
  }
  return true;
}

}  // namespace perfetto
