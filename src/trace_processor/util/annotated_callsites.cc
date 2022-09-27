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

#include "src/trace_processor/util/annotated_callsites.h"

#include <iostream>

#include "perfetto/ext/base/optional.h"
#include "src/trace_processor/tables/profiler_tables.h"
#include "src/trace_processor/types/trace_processor_context.h"

namespace perfetto {
namespace trace_processor {

AnnotatedCallsites::AnnotatedCallsites(const TraceProcessorContext* context)
    : context_(*context),
      // String to identify trampoline frames. If the string does not exist in
      // TraceProcessor's StringPool (nullopt) then there will be no trampoline
      // frames in the trace so there is no point in adding it to the pool to do
      // all comparisons, instead we initialize the member to nullopt and the
      // string comparisons will all fail.
      art_jni_trampoline_(
          context->storage->string_pool().GetId("art_jni_trampoline")) {}

AnnotatedCallsites::State AnnotatedCallsites::GetState(
    base::Optional<CallsiteId> id) {
  if (!id) {
    return State::kInitial;
  }
  auto it = states_.find(*id);
  if (it != states_.end()) {
    return it->second;
  }

  State state =
      Get(*context_.storage->stack_profile_callsite_table().FindById(*id))
          .first;
  states_.emplace(*id, state);
  return state;
}

std::pair<AnnotatedCallsites::State, CallsiteAnnotation>
AnnotatedCallsites::Get(
    const tables::StackProfileCallsiteTable::ConstRowReference& callsite) {
  State state = GetState(callsite.parent_id());

  // Keep immediate callee of a JNI trampoline, but keep tagging all
  // successive libart frames as common.
  if (state == State::kKeepNext) {
    return {State::kEraseLibart, CallsiteAnnotation::kNone};
  }

  // Special-case "art_jni_trampoline" frames, keeping their immediate callee
  // even if it is in libart, as it could be a native implementation of a
  // managed method. Example for "java.lang.reflect.Method.Invoke":
  //   art_jni_trampoline
  //   art::Method_invoke(_JNIEnv*, _jobject*, _jobject*, _jobjectArray*)
  //
  // Simpleperf also relies on this frame name, so it should be fairly stable.
  // TODO(rsavitski): consider detecting standard JNI upcall entrypoints -
  // _JNIEnv::Call*. These are sometimes inlined into other DSOs, so erasing
  // only the libart frames does not clean up all of the JNI-related frames.
  auto frame = *context_.storage->stack_profile_frame_table().FindById(
      callsite.frame_id());
  // art_jni_trampoline_ could be nullopt if the string does not exist in the
  // StringPool, but that also means no frame will ever have that name.
  if (art_jni_trampoline_.has_value() &&
      frame.name() == art_jni_trampoline_.value()) {
    return {State::kKeepNext, CallsiteAnnotation::kCommonFrame};
  }

  MapType map_type = GetMapType(frame.mapping());

  // Annotate managed frames.
  if (map_type == MapType::kArtInterp ||  //
      map_type == MapType::kArtJit ||     //
      map_type == MapType::kArtAot) {
    // Now know to be in a managed callstack - erase subsequent ART frames.
    if (state == State::kInitial) {
      state = State::kEraseLibart;
    }

    if (map_type == MapType::kArtInterp)
      return {state, CallsiteAnnotation::kArtInterpreted};
    if (map_type == MapType::kArtJit)
      return {state, CallsiteAnnotation::kArtJit};
    if (map_type == MapType::kArtAot)
      return {state, CallsiteAnnotation::kArtAot};
  }

  if (state == State::kEraseLibart && map_type == MapType::kNativeLibart) {
    states_.emplace(callsite.id(), state);
    return {state, CallsiteAnnotation::kCommonFrame};
  }

  return {state, CallsiteAnnotation::kNone};
}

AnnotatedCallsites::MapType AnnotatedCallsites::GetMapType(MappingId id) {
  auto it = map_types_.find(id);
  if (it != map_types_.end()) {
    return it->second;
  }

  return map_types_
      .emplace(id, ClassifyMap(context_.storage->GetString(
                       context_.storage->stack_profile_mapping_table()
                           .FindById(id)
                           ->name())))
      .first->second;
}

AnnotatedCallsites::MapType AnnotatedCallsites::ClassifyMap(
    NullTermStringView map) {
  if (map.empty())
    return MapType::kOther;

  // Primary mapping where modern ART puts jitted code.
  // TODO(rsavitski): look into /memfd:jit-zygote-cache.
  if (!strncmp(map.c_str(), "/memfd:jit-cache", 16))
    return MapType::kArtJit;

  size_t last_slash_pos = map.rfind('/');
  if (last_slash_pos != NullTermStringView::npos) {
    if (!strncmp(map.c_str() + last_slash_pos, "/libart.so", 10))
      return MapType::kNativeLibart;
    if (!strncmp(map.c_str() + last_slash_pos, "/libartd.so", 11))
      return MapType::kNativeLibart;
  }

  size_t extension_pos = map.rfind('.');
  if (extension_pos != NullTermStringView::npos) {
    if (!strncmp(map.c_str() + extension_pos, ".so", 3))
      return MapType::kNativeOther;
    // dex with verification speedup info, produced by dex2oat
    if (!strncmp(map.c_str() + extension_pos, ".vdex", 5))
      return MapType::kArtInterp;
    // possibly uncompressed dex in a jar archive
    if (!strncmp(map.c_str() + extension_pos, ".jar", 4))
      return MapType::kArtInterp;
    // ahead of time compiled ELFs
    if (!strncmp(map.c_str() + extension_pos, ".oat", 4))
      return MapType::kArtAot;
    // older/alternative name for .oat
    if (!strncmp(map.c_str() + extension_pos, ".odex", 5))
      return MapType::kArtAot;
  }
  return MapType::kOther;
}

}  // namespace trace_processor
}  // namespace perfetto
