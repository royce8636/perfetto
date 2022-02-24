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

#ifndef TEST_TEST_HELPER_H_
#define TEST_TEST_HELPER_H_

#include <stdio.h>
#include <stdlib.h>

#include "perfetto/base/build_config.h"
#include "perfetto/ext/base/file_utils.h"
#include "perfetto/ext/base/optional.h"
#include "perfetto/ext/base/scoped_file.h"
#include "perfetto/ext/base/subprocess.h"
#include "perfetto/ext/base/thread_task_runner.h"
#include "perfetto/ext/base/utils.h"
#include "perfetto/ext/tracing/core/consumer.h"
#include "perfetto/ext/tracing/core/shared_memory_arbiter.h"
#include "perfetto/ext/tracing/core/trace_packet.h"
#include "perfetto/ext/tracing/ipc/consumer_ipc_client.h"
#include "perfetto/ext/tracing/ipc/default_socket.h"
#include "perfetto/ext/tracing/ipc/service_ipc_host.h"
#include "perfetto/tracing/core/trace_config.h"
#include "src/base/test/test_task_runner.h"
#include "test/fake_producer.h"

#if PERFETTO_BUILDFLAG(PERFETTO_OS_WIN)
#include "src/tracing/ipc/shared_memory_windows.h"
#else
#include "src/traced/probes/probes_producer.h"
#include "src/tracing/ipc/posix_shared_memory.h"
#endif

#include "protos/perfetto/trace/trace_packet.gen.h"

namespace perfetto {

// This value has been bumped to 10s in Oct 2020 because the GCE-based emulator
// can be sensibly slower than real hw (more than 10x) and caused flakes.
// See bugs duped against b/171771440.
constexpr uint32_t kDefaultTestTimeoutMs = 10000;

inline const char* GetTestProducerSockName() {
// If we're building on Android and starting the daemons ourselves,
// create the sockets in a world-writable location.
#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID) && \
    PERFETTO_BUILDFLAG(PERFETTO_START_DAEMONS)
  return "/data/local/tmp/traced_producer";
#else
  return ::perfetto::GetProducerSocket();
#endif
}

// This is used only in daemon starting integrations tests.
class ServiceThread {
 public:
  ServiceThread(const std::string& producer_socket,
                const std::string& consumer_socket)
      : producer_socket_(producer_socket), consumer_socket_(consumer_socket) {}

  ~ServiceThread() {
    if (!runner_)
      return;
    runner_->PostTaskAndWaitForTesting([this]() { svc_.reset(); });
  }

  void Start() {
    runner_ = base::ThreadTaskRunner::CreateAndStart("perfetto.svc");
    runner_->PostTaskAndWaitForTesting([this]() {
      svc_ = ServiceIPCHost::CreateInstance(runner_->get());
      if (remove(producer_socket_.c_str()) == -1) {
        if (errno != ENOENT)
          PERFETTO_FATAL("Failed to remove %s", producer_socket_.c_str());
      }
      if (remove(consumer_socket_.c_str()) == -1) {
        if (errno != ENOENT)
          PERFETTO_FATAL("Failed to remove %s", consumer_socket_.c_str());
      }
      base::SetEnv("PERFETTO_PRODUCER_SOCK_NAME", producer_socket_);
      base::SetEnv("PERFETTO_CONSUMER_SOCK_NAME", consumer_socket_);
      bool res =
          svc_->Start(producer_socket_.c_str(), consumer_socket_.c_str());
      if (!res) {
        PERFETTO_FATAL("Failed to start service listening on %s and %s",
                       producer_socket_.c_str(), consumer_socket_.c_str());
      }
    });
  }

  base::ThreadTaskRunner* runner() { return runner_ ? &*runner_ : nullptr; }

 private:
  base::Optional<base::ThreadTaskRunner> runner_;  // Keep first.

  std::string producer_socket_;
  std::string consumer_socket_;
  std::unique_ptr<ServiceIPCHost> svc_;
};

// This is used only in daemon starting integrations tests.
#if PERFETTO_BUILDFLAG(PERFETTO_OS_WIN)
// On Windows we don't have any traced_probes, make this a no-op to avoid
// propagating #ifdefs to the outer test.
class ProbesProducerThread {
 public:
  ProbesProducerThread(const std::string& /*producer_socket*/) {}
  void Connect() {}
};
#else
class ProbesProducerThread {
 public:
  ProbesProducerThread(const std::string& producer_socket)
      : producer_socket_(producer_socket) {}

  ~ProbesProducerThread() {
    if (!runner_)
      return;
    runner_->PostTaskAndWaitForTesting([this]() { producer_.reset(); });
  }

  void Connect() {
    runner_ = base::ThreadTaskRunner::CreateAndStart("perfetto.prd.probes");
    runner_->PostTaskAndWaitForTesting([this]() {
      producer_.reset(new ProbesProducer());
      producer_->ConnectWithRetries(producer_socket_.c_str(), runner_->get());
    });
  }

 private:
  base::Optional<base::ThreadTaskRunner> runner_;  // Keep first.

  std::string producer_socket_;
  std::unique_ptr<ProbesProducer> producer_;
};
#endif  // !OS_WIN

class FakeProducerThread {
 public:
  FakeProducerThread(const std::string& producer_socket,
                     std::function<void()> connect_callback,
                     std::function<void()> setup_callback,
                     std::function<void()> start_callback)
      : producer_socket_(producer_socket),
        connect_callback_(std::move(connect_callback)),
        setup_callback_(std::move(setup_callback)),
        start_callback_(std::move(start_callback)) {
    runner_ = base::ThreadTaskRunner::CreateAndStart("perfetto.prd.fake");
    runner_->PostTaskAndWaitForTesting([this]() {
      producer_.reset(
          new FakeProducer("android.perfetto.FakeProducer", runner_->get()));
    });
  }

  ~FakeProducerThread() {
    runner_->PostTaskAndWaitForTesting([this]() { producer_.reset(); });
  }

  void Connect() {
    runner_->PostTaskAndWaitForTesting([this]() {
      producer_->Connect(producer_socket_.c_str(), std::move(connect_callback_),
                         std::move(setup_callback_), std::move(start_callback_),
                         std::move(shm_), std::move(shm_arbiter_));
    });
  }

  base::ThreadTaskRunner* runner() { return runner_ ? &*runner_ : nullptr; }

  FakeProducer* producer() { return producer_.get(); }

  void CreateProducerProvidedSmb() {
#if PERFETTO_BUILDFLAG(PERFETTO_OS_WIN)
    SharedMemoryWindows::Factory factory;
#else
    PosixSharedMemory::Factory factory;
#endif
    shm_ = factory.CreateSharedMemory(1024 * 1024);
    shm_arbiter_ = SharedMemoryArbiter::CreateUnboundInstance(shm_.get(), 4096);
  }

  void ProduceStartupEventBatch(const protos::gen::TestConfig& config,
                                std::function<void()> callback) {
    PERFETTO_CHECK(shm_arbiter_);
    producer_->ProduceStartupEventBatch(config, shm_arbiter_.get(), callback);
  }

 private:
  base::Optional<base::ThreadTaskRunner> runner_;  // Keep first.

  std::string producer_socket_;
  std::unique_ptr<FakeProducer> producer_;
  std::function<void()> connect_callback_;
  std::function<void()> setup_callback_;
  std::function<void()> start_callback_;
  std::unique_ptr<SharedMemory> shm_;
  std::unique_ptr<SharedMemoryArbiter> shm_arbiter_;
};

class TestHelper : public Consumer {
 public:
  enum class Mode {
    kStartDaemons,
    kUseSystemService,
  };
  static Mode kDefaultMode;

  static const char* GetDefaultModeConsumerSocketName();
  static const char* GetDefaultModeProducerSocketName();

  explicit TestHelper(base::TestTaskRunner* task_runner)
      : TestHelper(task_runner, kDefaultMode) {}

  explicit TestHelper(base::TestTaskRunner* task_runner, Mode mode);

  // Consumer implementation.
  void OnConnect() override;
  void OnDisconnect() override;
  void OnTracingDisabled(const std::string& error) override;
  virtual void ReadTraceData(std::vector<TracePacket> packets);
  void OnTraceData(std::vector<TracePacket> packets, bool has_more) override;
  void OnDetach(bool) override;
  void OnAttach(bool, const TraceConfig&) override;
  void OnTraceStats(bool, const TraceStats&) override;
  void OnObservableEvents(const ObservableEvents&) override;

  // Starts the tracing service if in kStartDaemons mode.
  void StartServiceIfRequired();

  // Connects the producer and waits that the service has seen the
  // RegisterDataSource() call.
  FakeProducer* ConnectFakeProducer();

  void ConnectConsumer();
  void StartTracing(const TraceConfig& config,
                    base::ScopedFile = base::ScopedFile());
  void DisableTracing();
  void FlushAndWait(uint32_t timeout_ms);
  void ReadData(uint32_t read_count = 0);
  void FreeBuffers();
  void DetachConsumer(const std::string& key);
  bool AttachConsumer(const std::string& key);
  bool SaveTraceForBugreportAndWait();
  void CreateProducerProvidedSmb();
  bool IsShmemProvidedByProducer();
  void ProduceStartupEventBatch(const protos::gen::TestConfig& config);

  void WaitFor(std::function<bool()> predicate,
               const std::string& error_msg,
               uint32_t timeout_ms = kDefaultTestTimeoutMs);
  void WaitForConsumerConnect();
  void WaitForProducerSetup();
  void WaitForProducerEnabled();
  void WaitForDataSourceConnected(const std::string& ds_name);
  void WaitForTracingDisabled(uint32_t timeout_ms = kDefaultTestTimeoutMs);
  void WaitForReadData(uint32_t read_count = 0,
                       uint32_t timeout_ms = kDefaultTestTimeoutMs);
  void SyncAndWaitProducer();
  TracingServiceState QueryServiceStateAndWait();

  std::string AddID(const std::string& checkpoint) {
    return checkpoint + "." + std::to_string(instance_num_);
  }

  std::function<void()> CreateCheckpoint(const std::string& checkpoint) {
    return task_runner_->CreateCheckpoint(AddID(checkpoint));
  }

  void RunUntilCheckpoint(const std::string& checkpoint,
                          uint32_t timeout_ms = kDefaultTestTimeoutMs) {
    return task_runner_->RunUntilCheckpoint(AddID(checkpoint), timeout_ms);
  }

  std::function<void()> WrapTask(const std::function<void()>& function);

  base::ThreadTaskRunner* service_thread() { return service_thread_.runner(); }
  base::ThreadTaskRunner* producer_thread() {
    return fake_producer_thread_.runner();
  }
  const std::vector<protos::gen::TracePacket>& full_trace() {
    return full_trace_;
  }
  const std::vector<protos::gen::TracePacket>& trace() { return trace_; }

 private:
  static uint64_t next_instance_num_;
  uint64_t instance_num_;
  base::TestTaskRunner* task_runner_ = nullptr;
  int cur_consumer_num_ = 0;
  uint64_t trace_count_ = 0;

  std::function<void()> on_connect_callback_;
  std::function<void()> on_packets_finished_callback_;
  std::function<void()> on_stop_tracing_callback_;
  std::function<void()> on_detach_callback_;
  std::function<void(bool)> on_attach_callback_;

  std::vector<protos::gen::TracePacket> full_trace_;
  std::vector<protos::gen::TracePacket> trace_;

  Mode mode_;
  const char* producer_socket_;
  const char* consumer_socket_;
  ServiceThread service_thread_;
  FakeProducerThread fake_producer_thread_;

  std::unique_ptr<TracingService::ConsumerEndpoint> endpoint_;  // Keep last.
};

#if !PERFETTO_BUILDFLAG(PERFETTO_OS_WIN)

// This class is a reference to a child process that has in essence been execv
// to the requested binary. The process will start and then wait for Run()
// before proceeding. We use this to fork new processes before starting any
// additional threads in the parent process (otherwise you would risk
// deadlocks), but pause the forked processes until remaining setup (including
// any necessary threads) in the parent process is complete.
class Exec {
 public:
  // Starts the forked process that was created. If not null then |stderr_out|
  // will contain the stderr of the process.
  int Run(std::string* stderr_out = nullptr) {
    // We can't be the child process.
    PERFETTO_CHECK(getpid() != subprocess_.pid());
    // Will cause the entrypoint to continue.
    PERFETTO_CHECK(write(*sync_pipe_.wr, "1", 1) == 1);
    sync_pipe_.wr.reset();
    subprocess_.Wait();

    if (stderr_out) {
      *stderr_out = std::move(subprocess_.output());
    } else {
      PERFETTO_LOG("Child proc %d exited with stderr: \"%s\"",
                   subprocess_.pid(), subprocess_.output().c_str());
    }
    return subprocess_.returncode();
  }

  Exec(const std::string& argv0,
       std::initializer_list<std::string> args,
       std::string input = "") {
    subprocess_.args.stderr_mode = base::Subprocess::OutputMode::kBuffer;
    subprocess_.args.stdout_mode = base::Subprocess::OutputMode::kDevNull;
    subprocess_.args.input = input;

#if PERFETTO_BUILDFLAG(PERFETTO_START_DAEMONS)
    constexpr bool kUseSystemBinaries = false;
#else
    constexpr bool kUseSystemBinaries = true;
#endif

    std::vector<std::string>& cmd = subprocess_.args.exec_cmd;
    if (kUseSystemBinaries) {
      PERFETTO_CHECK(TestHelper::kDefaultMode ==
                     TestHelper::Mode::kUseSystemService);
      cmd.push_back("/system/bin/" + argv0);
      cmd.insert(cmd.end(), args.begin(), args.end());
    } else {
      PERFETTO_CHECK(TestHelper::kDefaultMode ==
                     TestHelper::Mode::kStartDaemons);
      subprocess_.args.env.push_back(
          std::string("PERFETTO_PRODUCER_SOCK_NAME=") +
          TestHelper::GetDefaultModeProducerSocketName());
      subprocess_.args.env.push_back(
          std::string("PERFETTO_CONSUMER_SOCK_NAME=") +
          TestHelper::GetDefaultModeConsumerSocketName());
      cmd.push_back(base::GetCurExecutableDir() + "/" + argv0);
      cmd.insert(cmd.end(), args.begin(), args.end());
    }

    if (!base::FileExists(cmd[0])) {
      PERFETTO_FATAL(
          "Cannot find %s. Make sure that the target has been built and, on "
          "Android, pushed to the device.",
          cmd[0].c_str());
    }

    // This pipe blocks the execution of the child process until the main test
    // process calls Run(). There are two conflicting problems here:
    // 1) We can't fork() subprocesses too late, because the test spawns threads
    //    for hosting the service. fork+threads = bad (see aosp/1089744).
    // 2) We can't run the subprocess too early, because we need to wait that
    //    the service threads are ready before trying to connect from the child
    //    process.
    sync_pipe_ = base::Pipe::Create();
    int sync_pipe_rd = *sync_pipe_.rd;
    subprocess_.args.preserve_fds.push_back(sync_pipe_rd);

    // This lambda will be called on the forked child process after having
    // setup pipe redirection and closed all FDs, right before the exec().
    // The Subprocesss harness will take care of closing also |sync_pipe_.wr|.
    subprocess_.args.posix_entrypoint_for_testing = [sync_pipe_rd] {
      // Don't add any logging here, all file descriptors are closed and trying
      // to log will likely cause undefined behaviors.
      char ignored = 0;
      PERFETTO_CHECK(PERFETTO_EINTR(read(sync_pipe_rd, &ignored, 1)) > 0);
      PERFETTO_CHECK(close(sync_pipe_rd) == 0 || errno == EINTR);
    };

    subprocess_.Start();
    sync_pipe_.rd.reset();
  }

 private:
  base::Subprocess subprocess_;
  base::Pipe sync_pipe_;
};

#endif  // !PERFETTO_BUILDFLAG(PERFETTO_OS_WIN)

}  // namespace perfetto

#endif  // TEST_TEST_HELPER_H_
