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

#ifndef SRC_PROFILING_MEMORY_HEAPPROFD_PRODUCER_H_
#define SRC_PROFILING_MEMORY_HEAPPROFD_PRODUCER_H_

#include <functional>
#include <map>

#include "perfetto/base/task_runner.h"

#include "perfetto/tracing/core/basic_types.h"
#include "perfetto/tracing/core/data_source_config.h"
#include "perfetto/tracing/core/producer.h"
#include "perfetto/tracing/core/tracing_service.h"

#include "src/profiling/memory/bounded_queue.h"
#include "src/profiling/memory/proc_utils.h"
#include "src/profiling/memory/process_matcher.h"
#include "src/profiling/memory/socket_listener.h"
#include "src/profiling/memory/system_property.h"

namespace perfetto {
namespace profiling {

// TODO(rsavitski): central daemon can do less work if it knows that the global
// operating mode is fork-based, as it then will not be interacting with the
// clients. This can be implemented as an additional mode here.
enum class HeapprofdMode { kCentral, kChild };

class HeapprofdProducer : public Producer {
 public:
  HeapprofdProducer(HeapprofdMode mode, base::TaskRunner* task_runner);
  ~HeapprofdProducer() override;

  // Producer Impl:
  void OnConnect() override;
  void OnDisconnect() override;
  void SetupDataSource(DataSourceInstanceID, const DataSourceConfig&) override;
  void StartDataSource(DataSourceInstanceID, const DataSourceConfig&) override;
  void StopDataSource(DataSourceInstanceID) override;
  void OnTracingSetup() override;
  void Flush(FlushRequestID,
             const DataSourceInstanceID* data_source_ids,
             size_t num_data_sources) override;

  // TODO(fmayer): Delete once we have generic reconnect logic.
  void ConnectWithRetries(const char* socket_name);
  void DumpAll();

  // Valid only if mode_ == kChild. Adopts the (connected) sockets inherited
  // from the target process, invoking the on-connection callback.
  void AdoptConnectedSockets(std::vector<base::ScopedFile> inherited_sockets);

  // Valid only if mode_ == kChild.
  void SetTargetProcess(pid_t target_pid, std::string target_cmdline);

 private:
  // TODO(fmayer): Delete once we have generic reconnect logic.
  enum State {
    kNotStarted = 0,
    kNotConnected,
    kConnecting,
    kConnected,
  };
  void Connect();
  void Restart();
  void ResetConnectionBackoff();
  void IncreaseConnectionBackoff();

  // TODO(fmayer): Delete once we have generic reconnect logic.
  State state_ = kNotStarted;
  uint32_t connection_backoff_ms_ = 0;
  const char* producer_sock_name_ = nullptr;

  const HeapprofdMode mode_;

  std::function<void(UnwindingRecord)> MakeSocketListenerCallback();
  std::vector<BoundedQueue<UnwindingRecord>> MakeUnwinderQueues(size_t n);
  std::vector<std::thread> MakeUnwindingThreads(size_t n);

  void FinishDataSourceFlush(FlushRequestID flush_id);
  bool Dump(DataSourceInstanceID id,
            FlushRequestID flush_id,
            bool has_flush_id);
  void DoContinuousDump(DataSourceInstanceID id, uint32_t dump_interval);

  // functionality specific to mode_ == kCentral
  std::unique_ptr<base::UnixSocket> MakeListeningSocket();

  // functionality specific to mode_ == kChild
  void TerminateProcess(int exit_status);
  bool SourceMatchesTarget(const HeapprofdConfig& cfg);

  struct DataSource {
    // This is a shared ptr so we can lend a weak_ptr to the bookkeeping
    // thread for unwinding.
    std::shared_ptr<TraceWriter> trace_writer;
    // These are opaque handles that shut down the sockets in SocketListener
    // once they go away.
    ProcessMatcher::ProcessSetSpecHandle processes;
    std::vector<SystemProperties::Handle> properties;
  };

  std::map<DataSourceInstanceID, DataSource> data_sources_;
  std::map<FlushRequestID, size_t> flushes_in_progress_;

  // These two are borrowed from the caller.
  base::TaskRunner* const task_runner_;
  std::unique_ptr<TracingService::ProducerEndpoint> endpoint_;

  BoundedQueue<BookkeepingRecord> bookkeeping_queue_;
  BookkeepingThread bookkeeping_thread_;
  std::thread bookkeeping_th_;
  std::vector<BoundedQueue<UnwindingRecord>> unwinder_queues_;
  std::vector<std::thread> unwinding_threads_;
  SocketListener socket_listener_;

  // state specific to mode_ == kCentral
  std::unique_ptr<base::UnixSocket> listening_socket_;
  SystemProperties properties_;

  // state specific to mode_ == kChild
  pid_t target_pid_ = base::kInvalidPid;
  std::string target_cmdline_;

  base::WeakPtrFactory<HeapprofdProducer> weak_factory_;
};

}  // namespace profiling
}  // namespace perfetto

#endif  // SRC_PROFILING_MEMORY_HEAPPROFD_PRODUCER_H_
