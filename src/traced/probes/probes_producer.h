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

#include <map>
#include <memory>
#include <utility>

#include "perfetto/base/task_runner.h"
#include "perfetto/ftrace_reader/ftrace_controller.h"
#include "perfetto/tracing/core/producer.h"
#include "perfetto/tracing/core/trace_writer.h"
#include "perfetto/tracing/ipc/producer_ipc_client.h"

#ifndef SRC_TRACED_PROBES_PROBES_PRODUCER_H_
#define SRC_TRACED_PROBES_PROBES_PRODUCER_H_

namespace perfetto {
class ProbesProducer : public Producer {
 public:
  ~ProbesProducer() override;

  // Producer Impl:
  void OnConnect() override;
  void OnDisconnect() override;
  void CreateDataSourceInstance(DataSourceInstanceID,
                                const DataSourceConfig&) override;
  void TearDownDataSourceInstance(DataSourceInstanceID) override;

  // Our Impl
  void ConnectWithRetries(const char* socket_name,
                          base::TaskRunner* task_runner);
  void CreateFtraceDataSourceInstance(DataSourceInstanceID id,
                                      const DataSourceConfig& source_config);
  void CreateProcessStatsDataSourceInstance(
      const DataSourceConfig& source_config);

 private:
  using FtraceBundleHandle =
      protozero::MessageHandle<protos::pbzero::FtraceEventBundle>;

  class SinkDelegate : public FtraceSink::Delegate {
   public:
    explicit SinkDelegate(std::unique_ptr<TraceWriter> writer);
    ~SinkDelegate() override;

    // FtraceDelegateImpl
    FtraceBundleHandle GetBundleForCpu(size_t cpu) override;
    void OnBundleComplete(size_t cpu, FtraceBundleHandle bundle) override;

    void sink(std::unique_ptr<FtraceSink> sink) { sink_ = std::move(sink); }

   private:
    std::unique_ptr<FtraceSink> sink_ = nullptr;
    std::unique_ptr<TraceWriter> writer_;

    // Keep this after the TraceWriter because TracePackets must not outlive
    // their originating writer.
    TraceWriter::TracePacketHandle trace_packet_;
  };

  enum State {
    kNotStarted = 0,
    kNotConnected,
    kConnecting,
    kConnected,
  };

  void Connect();
  void ResetConnectionBackoff();
  void IncreaseConnectionBackoff();

  State state_ = kNotStarted;
  base::TaskRunner* task_runner_;
  std::unique_ptr<Service::ProducerEndpoint> endpoint_ = nullptr;
  std::unique_ptr<FtraceController> ftrace_ = nullptr;
  bool ftrace_creation_failed_ = false;
  uint64_t connection_backoff_ms_ = 0;
  const char* socket_name_ = nullptr;
  // Keeps track of id for each type of data source.
  std::map<DataSourceInstanceID, std::string> instances_;
  std::map<DataSourceInstanceID, std::unique_ptr<SinkDelegate>> delegates_;
  std::map<DataSourceInstanceID, base::WatchDog> watchdogs_;
};
}  // namespace perfetto

#endif  // SRC_TRACED_PROBES_PROBES_PRODUCER_H_
