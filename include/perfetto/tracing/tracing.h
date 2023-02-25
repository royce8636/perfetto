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

#ifndef INCLUDE_PERFETTO_TRACING_TRACING_H_
#define INCLUDE_PERFETTO_TRACING_TRACING_H_

#include <stddef.h>
#include <stdint.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "perfetto/base/compiler.h"
#include "perfetto/base/export.h"
#include "perfetto/base/logging.h"
#include "perfetto/tracing/backend_type.h"
#include "perfetto/tracing/core/forward_decls.h"
#include "perfetto/tracing/internal/in_process_tracing_backend.h"
#include "perfetto/tracing/internal/system_tracing_backend.h"
#include "perfetto/tracing/tracing_policy.h"

namespace perfetto {

namespace internal {
class TracingMuxerImpl;
}

class TracingBackend;
class Platform;
class StartupTracingSession;  // Declared below.
class TracingSession;         // Declared below.

struct TracingError {
  enum ErrorCode : uint32_t {
    // Peer disconnection.
    kDisconnected = 1,

    // The Start() method failed. This is typically because errors in the passed
    // TraceConfig. More details are available in |message|.
    kTracingFailed = 2,
  };

  ErrorCode code;
  std::string message;

  TracingError(ErrorCode cd, std::string msg)
      : code(cd), message(std::move(msg)) {
    PERFETTO_CHECK(!message.empty());
  }
};

using LogLev = ::perfetto::base::LogLev;
using LogMessageCallbackArgs = ::perfetto::base::LogMessageCallbackArgs;
using LogMessageCallback = ::perfetto::base::LogMessageCallback;

struct TracingInitArgs {
  uint32_t backends = 0;                     // One or more BackendTypes.
  TracingBackend* custom_backend = nullptr;  // [Optional].

  // [Optional] Platform implementation. It allows the embedder to take control
  // of platform-specific bits like thread creation and TLS slot handling. If
  // not set it will use Platform::GetDefaultPlatform().
  Platform* platform = nullptr;

  // [Optional] Tune the size of the shared memory buffer between the current
  // process and the service backend(s). This is a trade-off between memory
  // footprint and the ability to sustain bursts of trace writes (see comments
  // in shared_memory_abi.h).
  // If set, the value must be a multiple of 4KB. The value can be ignored if
  // larger than kMaxShmSize (32MB) or not a multiple of 4KB.
  uint32_t shmem_size_hint_kb = 0;

  // [Optional] Specifies the preferred size of each page in the shmem buffer.
  // This is a trade-off between IPC overhead and fragmentation/efficiency of
  // the shmem buffer in presence of multiple writer threads.
  // Must be one of [4, 8, 16, 32].
  uint32_t shmem_page_size_hint_kb = 0;

  // [Optional] The length of the period during which shared-memory-buffer
  // chunks that have been filled with data are accumulated (batched) on the
  // producer side, before the service is notified of them over an out-of-band
  // IPC call. If, while this period lasts, the shared memory buffer gets too
  // full, the IPC call will be sent immediately. The value of this parameter is
  // a trade-off between IPC traffic overhead and the ability to sustain bursts
  // of trace writes. The higher the value, the more chunks will be batched and
  // the less buffer space will be available to hide the latency of the service,
  // and vice versa. For more details, see the SetBatchCommitsDuration method in
  // shared_memory_arbiter.h.
  //
  // Note: With the default value of 0ms, batching still happens but with a zero
  // delay, i.e. commits will be sent to the service at the next opportunity.
  uint32_t shmem_batch_commits_duration_ms = 0;

  // [Optional] If set, the policy object is notified when certain SDK events
  // occur and may apply policy decisions, such as denying connections. The
  // embedder is responsible for ensuring the object remains alive for the
  // lifetime of the process.
  TracingPolicy* tracing_policy = nullptr;

  // [Optional] If set, log messages generated by perfetto are passed to this
  // callback instead of being logged directly.
  LogMessageCallback log_message_callback = nullptr;

  // When this flag is set to false, it overrides
  // `DataSource::kSupportsMultipleInstances` for all the data sources.
  // As a result when a tracing session is already running and if we attempt to
  // start another session, it will fail to start the data source which were
  // already active.
  bool supports_multiple_data_source_instances = true;

  // If this flag is set the default clock for taking timestamps is overridden
  // with CLOCK_MONOTONIC (for use in Chrome).
  bool use_monotonic_clock = false;

  // If this flag is set the default clock for taking timestamps is overridden
  // with CLOCK_MONOTONIC_RAW on platforms that support it.
  bool use_monotonic_raw_clock = false;

  // This flag can be set to false in order to avoid enabling the system
  // consumer in Tracing::Initialize(), so that the linker can remove the unused
  // consumer IPC implementation to reduce binary size. This setting only has an
  // effect if kSystemBackend is specified in |backends|. When this option is
  // false, Tracing::NewTrace() will instatiate the system backend only if
  // explicitly specified as kSystemBackend: kUndefinedBackend will consider
  // only already instantiated backends.
  bool enable_system_consumer = true;

 protected:
  friend class Tracing;
  friend class internal::TracingMuxerImpl;

  using BackendFactoryFunction = TracingBackend* (*)();
  using ProducerBackendFactoryFunction = TracingProducerBackend* (*)();
  using ConsumerBackendFactoryFunction = TracingConsumerBackend* (*)();

  BackendFactoryFunction in_process_backend_factory_ = nullptr;
  ProducerBackendFactoryFunction system_producer_backend_factory_ = nullptr;
  ConsumerBackendFactoryFunction system_consumer_backend_factory_ = nullptr;
  bool dcheck_is_on_ = PERFETTO_DCHECK_IS_ON();
};

// The entry-point for using perfetto.
class PERFETTO_EXPORT_COMPONENT Tracing {
 public:
  // Initializes Perfetto with the given backends in the calling process and/or
  // with a user-provided backend. It's possible to call this function more than
  // once to initialize different backends. If a backend was already initialized
  // the call will have no effect on it. All the members of `args` will be
  // ignored in subsequent calls, except those require to initialize new
  // backends (`backends`, `enable_system_consumer`, `shmem_size_hint_kb`,
  // `shmem_page_size_hint_kb` and `shmem_batch_commits_duration_ms`).
  static inline void Initialize(const TracingInitArgs& args)
      PERFETTO_ALWAYS_INLINE {
    TracingInitArgs args_copy(args);
    // This code is inlined to allow dead-code elimination for unused backends.
    // This saves ~200 KB when not using the in-process backend (b/148198993).
    // The logic behind it is the following:
    // Nothing other than the code below references the two GetInstance()
    // methods. From a linker-graph viewpoint, those GetInstance() pull in many
    // other pieces of the codebase (e.g. InProcessTracingBackend pulls the
    // whole TracingServiceImpl, SystemTracingBackend pulls the whole //ipc
    // layer). Due to the inline, the compiler can see through the code and
    // realize that some branches are always not taken. When that happens, no
    // reference to the backends' GetInstance() is emitted and that allows the
    // linker GC to get rid of the entire set of dependencies.
    if (args.backends & kInProcessBackend) {
      args_copy.in_process_backend_factory_ =
          &internal::InProcessTracingBackend::GetInstance;
    }
    if (args.backends & kSystemBackend) {
      args_copy.system_producer_backend_factory_ =
          &internal::SystemProducerTracingBackend::GetInstance;
      if (args.enable_system_consumer) {
        args_copy.system_consumer_backend_factory_ =
            &internal::SystemConsumerTracingBackend::GetInstance;
      }
    }
    InitializeInternal(args_copy);
  }

  // Checks if tracing has been initialized by calling |Initialize|.
  static bool IsInitialized();

  // Start a new tracing session using the given tracing backend. Use
  // |kUnspecifiedBackend| to select an available backend automatically.
  static inline std::unique_ptr<TracingSession> NewTrace(
      BackendType backend = kUnspecifiedBackend) PERFETTO_ALWAYS_INLINE {
    // This code is inlined to allow dead-code elimination for unused consumer
    // implementation. The logic behind it is the following:
    // Nothing other than the code below references the GetInstance() method
    // below. From a linker-graph viewpoint, those GetInstance() pull in many
    // other pieces of the codebase (ConsumerOnlySystemTracingBackend pulls
    // ConsumerIPCClient). Due to the inline, the compiler can see through the
    // code and realize that some branches are always not taken. When that
    // happens, no reference to the backends' GetInstance() is emitted and that
    // allows the linker GC to get rid of the entire set of dependencies.
    TracingConsumerBackend* (*system_backend_factory)();
    system_backend_factory = nullptr;
    // In case PERFETTO_IPC is disabled, a fake system backend is used, which
    // always panics. NewTrace(kSystemBackend) should fail if PERFETTO_IPC is
    // diabled, not panic.
#if PERFETTO_BUILDFLAG(PERFETTO_IPC)
    if (backend & kSystemBackend) {
      system_backend_factory =
          &internal::SystemConsumerTracingBackend::GetInstance;
    }
#endif
    return NewTraceInternal(backend, system_backend_factory);
  }

  // Shut down Perfetto, releasing any allocated OS resources (threads, files,
  // sockets, etc.). Note that Perfetto cannot be reinitialized again in the
  // same process[1]. Instead, this function is meant for shutting down all
  // Perfetto-related code so that it can be safely unloaded, e.g., with
  // dlclose().
  //
  // It is only safe to call this function when all threads recording trace
  // events have been terminated or otherwise guaranteed to not make any further
  // calls into Perfetto.
  //
  // [1] Unless static data is also cleared through other means.
  static void Shutdown();

  // Uninitialize Perfetto. Only exposed for testing scenarios where it can be
  // guaranteed that no tracing sessions or other operations are happening when
  // this call is made.
  static void ResetForTesting();

  // Start a new startup tracing session in the current process. Startup tracing
  // can be used in anticipation of a session that will be started by the
  // specified backend in the near future. The data source configs in the
  // supplied TraceConfig have to (mostly) match those in the config that will
  // later be provided by the backend.
  // Learn more about config matching at ComputeStartupConfigHash.
  //
  // Note that startup tracing requires that either:
  //  (a) the service backend already has an SMB set up, or
  //  (b) the service backend to support producer-provided SMBs if the backend
  //      is not yet connected or no SMB has been set up yet
  //      (See `use_producer_provided_smb`). If necessary, the
  //      client library will briefly disconnect and reconnect the backend to
  //      supply an SMB to the backend. If the service does not accept the SMB,
  //      startup tracing will be aborted, but the service may still start the
  //      corresponding tracing session later.
  //
  // Startup tracing is NOT supported with the in-process backend. For this
  // backend, you can just start a regular tracing session and block until it is
  // set up instead.
  //
  // The client library will start the data sources instances specified in the
  // config with a placeholder target buffer. Once the backend starts a matching
  // tracing session, the session will resume as normal. If no matching session
  // is started after a timeout (or the backend doesn't accept the
  // producer-provided SMB), the startup tracing session will be aborted
  // and the data source instances stopped.
  struct OnStartupTracingSetupCallbackArgs {
    int num_data_sources_started;
  };
  struct SetupStartupTracingOpts {
    BackendType backend = kUnspecifiedBackend;
    uint32_t timeout_ms = 10000;

    // If set, this callback is executed (on an internal Perfetto thread) when
    // startup tracing was set up.
    std::function<void(OnStartupTracingSetupCallbackArgs)> on_setup;

    // If set, this callback is executed (on an internal Perfetto thread) if any
    // data sources were aborted, e.g. due to exceeding the timeout or as a
    // response to Abort().
    std::function<void()> on_aborted;

    // If set, this callback is executed (on an internal Perfetto thread) after
    // all data sources were adopted by a tracing session initiated by the
    // backend.
    std::function<void()> on_adopted;
  };

  static std::unique_ptr<StartupTracingSession> SetupStartupTracing(
      const TraceConfig& config,
      SetupStartupTracingOpts);

  // Blocking version of above method, so callers can ensure that tracing is
  // active before proceeding with app startup. Calls into
  // DataSource::Trace() or trace macros right after this method are written
  // into the startup session.
  static std::unique_ptr<StartupTracingSession> SetupStartupTracingBlocking(
      const TraceConfig& config,
      SetupStartupTracingOpts);

  // Informs the tracing services to activate any of these triggers if any
  // tracing session was waiting for them.
  //
  // Sends the trigger signal to all the initialized backends that are currently
  // connected and that connect in the next `ttl_ms` milliseconds (but
  // returns immediately anyway).
  static void ActivateTriggers(const std::vector<std::string>& triggers,
                               uint32_t ttl_ms);

 private:
  static void InitializeInternal(const TracingInitArgs&);
  static std::unique_ptr<TracingSession> NewTraceInternal(
      BackendType,
      TracingConsumerBackend* (*system_backend_factory)());

  Tracing() = delete;
};

class PERFETTO_EXPORT_COMPONENT TracingSession {
 public:
  virtual ~TracingSession();

  // Configure the session passing the trace config.
  // If a writable file handle is given through |fd|, the trace will
  // automatically written to that file. Otherwise you should call ReadTrace()
  // to retrieve the trace data. This call does not take ownership of |fd|.
  // TODO(primiano): add an error callback.
  virtual void Setup(const TraceConfig&, int fd = -1) = 0;

  // Enable tracing asynchronously. Use SetOnStartCallback() to get a
  // notification when the session has fully started.
  virtual void Start() = 0;

  // Enable tracing and block until tracing has started. Note that if data
  // sources are registered after this call was initiated, the call may return
  // before the additional data sources have started. Also, if other producers
  // (e.g., with system-wide tracing) have registered data sources without start
  // notification support, this call may return before those data sources have
  // started.
  virtual void StartBlocking() = 0;

  // This callback will be invoked when all data sources have acknowledged that
  // tracing has started. This callback will be invoked on an internal perfetto
  // thread.
  virtual void SetOnStartCallback(std::function<void()>) = 0;

  // This callback can be used to get a notification when some error occured
  // (e.g., peer disconnection). Error type will be passed as an argument. This
  // callback will be invoked on an internal perfetto thread.
  virtual void SetOnErrorCallback(std::function<void(TracingError)>) = 0;

  // Issues a flush request, asking all data sources to ack the request, within
  // the specified timeout. A "flush" is a fence to ensure visibility of data in
  // the async tracing pipeline. It guarantees that all data written before the
  // Flush() call will be visible in the trace buffer and hence by the
  // ReadTrace() / ReadTraceBlocking() methods.
  // Args:
  //  callback: will be invoked on an internal perfetto thread when all data
  //    sources have acked, or the timeout is reached. The bool argument
  //    will be true if all data sources acked within the timeout, false if
  //    the timeout was hit or some other error occurred (e.g. the tracing
  //    session wasn't started or ended).
  //  timeout_ms: how much time the service will wait for data source acks. If
  //    0, the global timeout specified in the TraceConfig (flush_timeout_ms)
  //    will be used. If flush_timeout_ms is also unspecified, a default value
  //    of 5s will be used.
  // Known issues:
  //    Because flushing is still based on service-side scraping, the very last
  //    trace packet for each data source thread will not be visible. Fixing
  //    this requires either propagating the Flush() to the data sources or
  //    changing the order of atomic operations in the service (b/162206162).
  //    Until then, a workaround is to make sure to call
  //    DataSource::Trace([](TraceContext ctx) { ctx.Flush(); }) just before
  //    stopping, on each thread where DataSource::Trace has been previously
  //    called.
  virtual void Flush(std::function<void(bool)>, uint32_t timeout_ms = 0) = 0;

  // Blocking version of Flush(). Waits until all data sources have acked and
  // returns the success/failure status.
  bool FlushBlocking(uint32_t timeout_ms = 0);

  // Disable tracing asynchronously.
  // Use SetOnStopCallback() to get a notification when the tracing session is
  // fully stopped and all data sources have acked.
  virtual void Stop() = 0;

  // Disable tracing and block until tracing has stopped.
  virtual void StopBlocking() = 0;

  // This callback will be invoked when tracing is disabled.
  // This can happen either when explicitly calling TracingSession.Stop() or
  // when the trace reaches its |duration_ms| time limit.
  // This callback will be invoked on an internal perfetto thread.
  virtual void SetOnStopCallback(std::function<void()>) = 0;

  // Changes the TraceConfig for an active tracing session. The session must
  // have been configured and started before. Note that the tracing service
  // only supports changing a subset of TraceConfig fields,
  // see ConsumerEndpoint::ChangeTraceConfig().
  virtual void ChangeTraceConfig(const TraceConfig&) = 0;

  // Struct passed as argument to the callback passed to ReadTrace().
  // [data, size] is guaranteed to contain 1 or more full trace packets, which
  // can be decoded using trace.proto. No partial or truncated packets are
  // exposed. If the trace is empty this returns a zero-sized nullptr with
  // |has_more| == true to signal EOF.
  // This callback will be invoked on an internal perfetto thread.
  struct ReadTraceCallbackArgs {
    const char* data = nullptr;
    size_t size = 0;

    // When false, this will be the last invocation of the callback for this
    // read cycle.
    bool has_more = false;
  };

  // Reads back the trace data (raw protobuf-encoded bytes) asynchronously.
  // Can be called at any point during the trace, typically but not necessarily,
  // after stopping. If this is called before the end of the trace (i.e. before
  // Stop() / StopBlocking()), in almost all cases you need to call
  // Flush() / FlushBlocking() before Read(). This is to guarantee that tracing
  // data in-flight in the data sources is committed into the tracing buffers
  // before reading them.
  // Reading the trace data is a destructive operation w.r.t. contents of the
  // trace buffer and is not idempotent.
  // A single ReadTrace() call can yield >1 callback invocations, until
  // |has_more| is false.
  using ReadTraceCallback = std::function<void(ReadTraceCallbackArgs)>;
  virtual void ReadTrace(ReadTraceCallback) = 0;

  // Synchronous version of ReadTrace(). It blocks the calling thread until all
  // the trace contents are read. This is slow and inefficient (involves more
  // copies) and is mainly intended for testing.
  std::vector<char> ReadTraceBlocking();

  // Struct passed as an argument to the callback for GetTraceStats(). Contains
  // statistics about the tracing session.
  struct GetTraceStatsCallbackArgs {
    // Whether or not querying statistics succeeded.
    bool success = false;
    // Serialized TraceStats protobuf message. To decode:
    //
    //   perfetto::protos::gen::TraceStats trace_stats;
    //   trace_stats.ParseFromArray(args.trace_stats_data.data(),
    //                              args.trace_stats_data.size());
    //
    std::vector<uint8_t> trace_stats_data;
  };

  // Requests a snapshot of statistical data for this tracing session. Only one
  // query may be active at a time. This callback will be invoked on an internal
  // perfetto thread.
  using GetTraceStatsCallback = std::function<void(GetTraceStatsCallbackArgs)>;
  virtual void GetTraceStats(GetTraceStatsCallback) = 0;

  // Synchronous version of GetTraceStats() for convenience.
  GetTraceStatsCallbackArgs GetTraceStatsBlocking();

  // Struct passed as an argument to the callback for QueryServiceState().
  // Contains information about registered data sources.
  struct QueryServiceStateCallbackArgs {
    // Whether or not getting the service state succeeded.
    bool success = false;
    // Serialized TracingServiceState protobuf message. To decode:
    //
    //   perfetto::protos::gen::TracingServiceState state;
    //   state.ParseFromArray(args.service_state_data.data(),
    //                        args.service_state_data.size());
    //
    std::vector<uint8_t> service_state_data;
  };

  // Requests a snapshot of the tracing service state for this session. Only one
  // request per session may be active at a time. This callback will be invoked
  // on an internal perfetto thread.
  using QueryServiceStateCallback =
      std::function<void(QueryServiceStateCallbackArgs)>;
  virtual void QueryServiceState(QueryServiceStateCallback) = 0;

  // Synchronous version of QueryServiceState() for convenience.
  QueryServiceStateCallbackArgs QueryServiceStateBlocking();
};

class PERFETTO_EXPORT_COMPONENT StartupTracingSession {
 public:
  // Note that destroying the StartupTracingSession object will not abort the
  // startup session automatically. Call Abort() explicitly to do so.
  virtual ~StartupTracingSession();

  // Abort any active but still unbound data source instances that belong to
  // this startup tracing session. Does not affect data source instances that
  // were already bound to a service-controlled session.
  virtual void Abort() = 0;

  // Same as above, but blocks the current thread until aborted.
  // Note some of the internal (non observable from public APIs) cleanup might
  // be done even after this method returns.
  virtual void AbortBlocking() = 0;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_TRACING_H_
