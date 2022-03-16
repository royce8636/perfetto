/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "perfetto/tracing/traced_proto.h"

#include "perfetto/test/traced_value_test_support.h"
#include "perfetto/tracing/track_event.h"
#include "protos/perfetto/trace/test_event.gen.h"
#include "protos/perfetto/trace/test_event.pb.h"
#include "protos/perfetto/trace/test_event.pbzero.h"
#include "protos/perfetto/trace/track_event/track_event.gen.h"
#include "protos/perfetto/trace/track_event/track_event.pb.h"
#include "test/gtest_and_gmock.h"

namespace perfetto {

class TracedProtoTest : public ::testing::Test {
 public:
  TracedProtoTest() : context_(track_event_.get(), &incremental_state_) {}

  EventContext& context() { return context_; }

 private:
  protozero::HeapBuffered<protos::pbzero::TrackEvent> track_event_;
  internal::TrackEventIncrementalState incremental_state_;
  EventContext context_;
};

using TestPayload = protos::pbzero::TestEvent::TestPayload;

TEST_F(TracedProtoTest, SingleInt) {
  protozero::HeapBuffered<TestPayload> event;
  WriteTracedProtoField(context().Wrap(event.get()), TestPayload::kSingleInt,
                        42);

  protos::TestEvent::TestPayload result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_TRUE(result.has_single_int());
  EXPECT_EQ(result.single_int(), 42);
}

TEST_F(TracedProtoTest, RepeatedInt) {
  protozero::HeapBuffered<TestPayload> event;
  WriteTracedProtoField(context().Wrap(event.get()), TestPayload::kRepeatedInts,
                        std::vector<int>{1, 2, 3});

  protos::TestEvent::TestPayload result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_THAT(result.repeated_ints(), ::testing::ElementsAre(1, 2, 3));
}

TEST_F(TracedProtoTest, SingleString) {
  protozero::HeapBuffered<TestPayload> event;
  WriteTracedProtoField(context().Wrap(event.get()), TestPayload::kSingleString,
                        "foo");

  protos::TestEvent::TestPayload result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_TRUE(result.has_single_string());
  EXPECT_EQ(result.single_string(), "foo");
}

TEST_F(TracedProtoTest, RepeatedString) {
  protozero::HeapBuffered<TestPayload> event;
  WriteTracedProtoField(context().Wrap(event.get()), TestPayload::kStr,
                        std::vector<std::string>{"foo", "bar"});

  protos::TestEvent::TestPayload result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_THAT(result.str(), ::testing::ElementsAre("foo", "bar"));
}

namespace {

struct Foo {
  void WriteIntoTrace(TracedProto<TestPayload> message) const {
    message->set_single_int(42);

    auto dict = std::move(message).AddDebugAnnotations();
    dict.Add("arg", "value");
  }
};

struct Bar {};

}  // namespace

template <>
struct TraceFormatTraits<Bar> {
  static void WriteIntoTrace(
      TracedProto<protos::pbzero::TestEvent::TestPayload> message,
      const Bar&) {
    message->set_single_string("value");
  }
};

TEST_F(TracedProtoTest, SingleNestedMessage_Method) {
  protozero::HeapBuffered<protos::pbzero::TestEvent> event;
  WriteTracedProtoField(context().Wrap(event.get()),
                        protos::pbzero::TestEvent::kPayload, Foo());

  protos::TestEvent result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_EQ(result.payload().single_int(), 42);
}

TEST_F(TracedProtoTest, SingleNestedMessage_TraceFormatTraits) {
  protozero::HeapBuffered<protos::pbzero::TestEvent> event;
  WriteTracedProtoField(context().Wrap(event.get()),
                        protos::pbzero::TestEvent::kPayload, Bar());

  protos::TestEvent result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_EQ(result.payload().single_string(), "value");
}

TEST_F(TracedProtoTest, SingleNestedMessage_Pointer) {
  protozero::HeapBuffered<protos::pbzero::TestEvent> event;
  Bar bar;
  WriteTracedProtoField(context().Wrap(event.get()),
                        protos::pbzero::TestEvent::kPayload, &bar);

  protos::TestEvent result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_EQ(result.payload().single_string(), "value");
}

TEST_F(TracedProtoTest, SingleNestedMessage_UniquePtr) {
  protozero::HeapBuffered<protos::pbzero::TestEvent> event;
  std::unique_ptr<Bar> bar(new Bar);
  WriteTracedProtoField(context().Wrap(event.get()),
                        protos::pbzero::TestEvent::kPayload, bar);

  protos::TestEvent result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_EQ(result.payload().single_string(), "value");
}

TEST_F(TracedProtoTest, SingleNestedMessage_EmptyUniquePtr) {
  protozero::HeapBuffered<protos::pbzero::TestEvent> event;
  std::unique_ptr<Bar> bar;
  WriteTracedProtoField(context().Wrap(event.get()),
                        protos::pbzero::TestEvent::kPayload, bar);

  protos::TestEvent result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_FALSE(result.payload().has_single_string());
}

TEST_F(TracedProtoTest, SingleNestedMessage_Nullptr) {
  protozero::HeapBuffered<protos::pbzero::TestEvent> event;
  WriteTracedProtoField(context().Wrap(event.get()),
                        protos::pbzero::TestEvent::kPayload, nullptr);

  protos::TestEvent result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_FALSE(result.payload().has_single_string());
}

TEST_F(TracedProtoTest, RepeatedNestedMessage_Method) {
  protozero::HeapBuffered<TestPayload> event;
  WriteTracedProtoField(context().Wrap(event.get()), TestPayload::kNested,
                        std::vector<Foo>{Foo(), Foo()});

  protos::TestEvent::TestPayload result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_EQ(result.nested_size(), 2);
  EXPECT_EQ(result.nested(0).single_int(), 42);
  EXPECT_EQ(result.nested(1).single_int(), 42);
}

TEST_F(TracedProtoTest, RepeatedNestedMessage_TraceFormatTraits) {
  protozero::HeapBuffered<TestPayload> event;
  WriteTracedProtoField(context().Wrap(event.get()), TestPayload::kNested,
                        std::vector<Bar>{Bar(), Bar()});

  protos::TestEvent::TestPayload result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_EQ(result.nested_size(), 2);
  EXPECT_EQ(result.nested(0).single_string(), "value");
  EXPECT_EQ(result.nested(1).single_string(), "value");
}

TEST_F(TracedProtoTest, RepeatedNestedMessage_Pointer) {
  protozero::HeapBuffered<TestPayload> event;
  Bar bar;
  std::vector<Bar*> bars;
  bars.push_back(&bar);
  bars.push_back(nullptr);
  WriteTracedProtoField(context().Wrap(event.get()), TestPayload::kNested,
                        bars);

  protos::TestEvent::TestPayload result;
  result.ParseFromString(event.SerializeAsString());
  EXPECT_EQ(result.nested_size(), 2);
  EXPECT_EQ(result.nested(0).single_string(), "value");
  EXPECT_FALSE(result.nested(1).has_single_string());
}

TEST_F(TracedProtoTest, WriteDebugAnnotations) {
  protozero::HeapBuffered<protos::pbzero::TestEvent> event;
  WriteTracedProtoField(context().Wrap(event.get()),
                        protos::pbzero::TestEvent::kPayload, Foo());

  protos::TestEvent result;
  result.ParseFromString(event.SerializeAsString());

  protos::DebugAnnotation dict;
  for (const auto& annotation : result.payload().debug_annotations()) {
    *dict.add_dict_entries() = annotation;
  }

  EXPECT_EQ(internal::DebugAnnotationToString(dict.SerializeAsString()),
            "{arg:value}");
}

}  // namespace perfetto
