/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "src/trace_processor/sqlite/pprof_functions.h"

#include "perfetto/trace_processor/status.h"
#include "src/trace_processor/sqlite/create_function_internal.h"
#include "src/trace_processor/types/trace_processor_context.h"
#include "src/trace_processor/util/profile_builder.h"

#include <cinttypes>
#include <limits>

// TODO(carlscab): We currently recreate the GProfileBuilder for every group. We
// should cache this somewhere maybe even have a helper table that stores all
// this data.

namespace perfetto {
namespace trace_processor {
namespace {

void SetSqliteError(sqlite3_context* ctx, const base::Status& status) {
  if (!status.ok()) {
    sqlite3_result_error(ctx, status.c_message(), -1);
  }
}

class PerfProfile {
 public:
  explicit PerfProfile(const TraceProcessorContext* context, bool annotated)
      : builder_(context, {{"samples", "count"}}, annotated) {
    single_count_value_.Append(1);
  }

  base::Status Step(uint32_t callsite_id) {
    builder_.AddSample(callsite_id, single_count_value_);
    return util::OkStatus();
  }

  base::Status Final(sqlite3_context* ctx) {
    // TODO(carlscab): A lot of copies are happening here.
    std::string profile_proto = builder_.Build();

    std::unique_ptr<uint8_t[], base::FreeDeleter> data(
        static_cast<uint8_t*>(malloc(profile_proto.size())));
    memcpy(data.get(), profile_proto.data(), profile_proto.size());
    sqlite3_result_blob(ctx, data.release(),
                        static_cast<int>(profile_proto.size()), free);
    return util::OkStatus();
  }

 private:
  GProfileBuilder builder_;
  protozero::PackedVarInt single_count_value_;
};

template <typename Function>
class ProfileFunction {
 public:
  static base::Status Register(sqlite3* db, TraceProcessorContext* context) {
    int flags = SQLITE_UTF8 | SQLITE_DETERMINISTIC;
    int ret = sqlite3_create_function_v2(db, Function::kName, 1, flags, context,
                                         nullptr, Step, Final, nullptr);
    if (ret != SQLITE_OK) {
      return base::ErrStatus("Unable to register function with name %s",
                             Function::kName);
    }
    return base::OkStatus();
  }

 private:
  static std::unique_ptr<PerfProfile> ReleaseProfile(sqlite3_context* ctx) {
    PerfProfile** profile =
        reinterpret_cast<PerfProfile**>(sqlite3_aggregate_context(ctx, 0));

    if (!profile) {
      return nullptr;
    }

    return std::unique_ptr<PerfProfile>(*profile);
  }

  static PerfProfile* GetOrCreateProfile(sqlite3_context* ctx) {
    PerfProfile** profile = reinterpret_cast<PerfProfile**>(
        sqlite3_aggregate_context(ctx, sizeof(PerfProfile*)));
    if (!profile) {
      return nullptr;
    }

    if (!*profile) {
      *profile = new PerfProfile(reinterpret_cast<const TraceProcessorContext*>(
                                     sqlite3_user_data(ctx)),
                                 Function::kAnnotate);
    }

    return *profile;
  }

  static void Step(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
      return SetSqliteError(ctx, base::ErrStatus("%s: invalid number of args; "
                                                 "expected 1, received %d",
                                                 Function::kName, argc));
    }

    base::Status status = TypeCheckSqliteValue(argv[0], SqlValue::kLong);
    if (!status.ok()) {
      return SetSqliteError(
          ctx, base::ErrStatus("%s: argument callsite_id %s", Function::kName,
                               status.c_message()));
    }
    int64_t value = sqlite3_value_int64(argv[0]);

    if (value < 0 || value > std::numeric_limits<uint32_t>::max()) {
      return SetSqliteError(ctx,
                            base::ErrStatus("%s: invalid callsite_id %" PRId64,
                                            Function::kName, value));
    }

    uint32_t callsite_id = static_cast<uint32_t>(value);

    PerfProfile* profile = GetOrCreateProfile(ctx);

    if (!profile) {
      return SetSqliteError(
          ctx, base::ErrStatus("%s: Failed to allocate aggregate context",
                               Function::kName));
    }

    status = profile->Step(callsite_id);

    if (!status.ok()) {
      return SetSqliteError(ctx, status);
    }
  }

  static void Final(sqlite3_context* ctx) {
    std::unique_ptr<PerfProfile> profile = ReleaseProfile(ctx);
    if (!profile) {
      return;
    }

    base::Status status = profile->Final(ctx);
    if (!status.ok()) {
      return SetSqliteError(ctx, status);
    }
  }
};

class PerfProfileFunction : public ProfileFunction<PerfProfileFunction> {
 public:
  static constexpr const char* kName = "EXPERIMENTAL_PERF_PROFILE";
  static constexpr bool kAnnotate = false;
};

class AnnotatedPerfProfileFunction
    : public ProfileFunction<AnnotatedPerfProfileFunction> {
 public:
  static constexpr const char* kName = "EXPERIMENTAL_ANNOTATED_PERF_PROFILE";
  static constexpr bool kAnnotate = true;
};

}  // namespace

base::Status PprofFunctions::Register(sqlite3* db,
                                      TraceProcessorContext* context) {
  auto status = PerfProfileFunction::Register(db, context);
  if (!status.ok()) {
    return status;
  }
  return AnnotatedPerfProfileFunction::Register(db, context);
}

}  // namespace trace_processor
}  // namespace perfetto
