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

#ifndef SRC_TRACED_PROBES_PACKAGES_LIST_PACKAGES_LIST_DATA_SOURCE_H_
#define SRC_TRACED_PROBES_PACKAGES_LIST_PACKAGES_LIST_DATA_SOURCE_H_

#include <functional>
#include <memory>

#include "perfetto/base/task_runner.h"

#include "perfetto/ext/tracing/core/basic_types.h"
#include "perfetto/trace/android/packages_list.pbzero.h"
#include "perfetto/tracing/core/data_source_config.h"

#include "src/traced/probes/probes_data_source.h"

namespace perfetto {

class TraceWriter;

struct Package {
  std::string name;
  uint64_t uid = 0;
  bool debuggable = false;
  bool profileable_from_shell = false;
  int64_t version_code = 0;
};

bool ReadPackagesListLine(char* line, Package* package);

class PackagesListDataSource : public ProbesDataSource {
 public:
  static constexpr int kTypeId = 7;
  PackagesListDataSource(TracingSessionID session_id,
                         std::unique_ptr<TraceWriter> writer);
  // ProbesDataSource implementation.
  void Start() override;
  void Flush(FlushRequestID, std::function<void()> callback) override;

  ~PackagesListDataSource() override;

 private:
  std::unique_ptr<TraceWriter> writer_;
};

}  // namespace perfetto

#endif  // SRC_TRACED_PROBES_PACKAGES_LIST_PACKAGES_LIST_DATA_SOURCE_H_
