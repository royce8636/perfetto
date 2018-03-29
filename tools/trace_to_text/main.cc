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

#include <inttypes.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <utility>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/field_comparator.h>
#include <google/protobuf/util/message_differencer.h>

#include "perfetto/base/logging.h"
#include "perfetto/trace/trace.pb.h"
#include "perfetto/trace/trace_packet.pb.h"
#include "tools/trace_to_text/ftrace_event_formatter.h"
#include "tools/trace_to_text/ftrace_inode_handler.h"

namespace perfetto {
namespace {

const char kTraceHeader[] = R"({
  "traceEvents": [],
)";

const char kTraceFooter[] = R"(\n",
  "controllerTraceDataKey": "systraceController"
})";

const char kFtraceHeader[] =
    ""
    "  \"systemTraceEvents\": \""
    "# tracer: nop\\n"
    "#\\n"
    "# entries-in-buffer/entries-written: 30624/30624   #P:4\\n"
    "#\\n"
    "#                                      _-----=> irqs-off\\n"
    "#                                     / _----=> need-resched\\n"
    "#                                    | / _---=> hardirq/softirq\\n"
    "#                                    || / _--=> preempt-depth\\n"
    "#                                    ||| /     delay\\n"
    "#           TASK-PID    TGID   CPU#  ||||    TIMESTAMP  FUNCTION\\n"
    "#              | |        |      |   ||||       |         |\\n";

using google::protobuf::Descriptor;
using google::protobuf::DynamicMessageFactory;
using google::protobuf::FileDescriptor;
using google::protobuf::Message;
using google::protobuf::TextFormat;
using google::protobuf::compiler::DiskSourceTree;
using google::protobuf::compiler::Importer;
using google::protobuf::compiler::MultiFileErrorCollector;
using google::protobuf::io::OstreamOutputStream;

using protos::FtraceEvent;
using protos::FtraceEventBundle;
using protos::InodeFileMap;
using protos::PrintFtraceEvent;
using protos::ProcessTree;
using protos::Trace;
using protos::TracePacket;
using Entry = protos::InodeFileMap::Entry;
using Process = protos::ProcessTree::Process;

// TODO(hjd): Add tests.

size_t GetWidth() {
  if (!isatty(STDOUT_FILENO))
    return 80;
  struct winsize win_size;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_size);
  return win_size.ws_col;
}

class MFE : public MultiFileErrorCollector {
  virtual void AddError(const std::string& filename,
                        int line,
                        int column,
                        const std::string& message) {
    PERFETTO_ELOG("Error %s %d:%d: %s", filename.c_str(), line, column,
                  message.c_str());
  }

  virtual void AddWarning(const std::string& filename,
                          int line,
                          int column,
                          const std::string& message) {
    PERFETTO_ELOG("Error %s %d:%d: %s", filename.c_str(), line, column,
                  message.c_str());
  }
};

void ForEachPacketInTrace(
    std::istream* input,
    const std::function<void(const protos::TracePacket&)>& f) {
  size_t bytes_processed = 0;
  // The trace stream can be very large. We cannot just pass it in one go to
  // libprotobuf as that will refuse to parse messages > 64MB. However we know
  // that a trace is merely a sequence of TracePackets. Here we just manually
  // tokenize the repeated TracePacket messages and parse them individually
  // using libprotobuf.
  for (;;) {
    fprintf(stderr, "Processing trace: %8zu KB\r", bytes_processed / 1024);
    fflush(stderr);
    // A TracePacket consists in one byte stating its field id and type ...
    char preamble;
    input->get(preamble);
    if (!input->good())
      break;
    bytes_processed++;
    PERFETTO_DCHECK(preamble == 0x0a);  // Field ID:1, type:length delimited.

    // ... a varint stating its size ...
    uint32_t field_size = 0;
    uint32_t shift = 0;
    for (;;) {
      char c = 0;
      input->get(c);
      field_size |= static_cast<uint32_t>(c & 0x7f) << shift;
      shift += 7;
      bytes_processed++;
      if (!(c & 0x80))
        break;
    }

    // ... and the actual TracePacket itself.
    std::unique_ptr<char[]> buf(new char[field_size]);
    input->read(buf.get(), field_size);
    bytes_processed += field_size;

    protos::TracePacket packet;
    PERFETTO_CHECK(packet.ParseFromArray(buf.get(), field_size));

    f(packet);
  }
}

int TraceToSystrace(std::istream* input, std::ostream* output) {
  std::multimap<uint64_t, std::string> sorted;

  ForEachPacketInTrace(input, [&sorted](const protos::TracePacket& packet) {
    if (!packet.has_ftrace_events())
      return;

    const FtraceEventBundle& bundle = packet.ftrace_events();
    for (const FtraceEvent& event : bundle.event()) {
      std::string line =
          FormatFtraceEvent(event.timestamp(), bundle.cpu(), event);
      if (line == "")
        continue;
      sorted.emplace(event.timestamp(), line);
    }
  });

  *output << kTraceHeader;
  *output << kFtraceHeader;

  fprintf(stderr, "\n");
  size_t total_events = sorted.size();
  size_t written_events = 0;
  for (auto it = sorted.begin(); it != sorted.end(); it++) {
    *output << it->second;
    if (written_events++ % 100 == 0) {
      fprintf(stderr, "Writing trace: %.2f %%\r",
              written_events * 100.0 / total_events);
      fflush(stderr);
    }
  }

  *output << kTraceFooter;

  return 0;
}

int TraceToText(std::istream* input, std::ostream* output) {
  DiskSourceTree dst;
  dst.MapPath("perfetto", "protos/perfetto");
  MFE mfe;
  Importer importer(&dst, &mfe);
  const FileDescriptor* parsed_file =
      importer.Import("perfetto/trace/trace.proto");

  DynamicMessageFactory dmf;
  const Descriptor* trace_descriptor = parsed_file->message_type(0);
  const Message* msg_root = dmf.GetPrototype(trace_descriptor);
  Message* msg = msg_root->New();

  if (!msg->ParseFromIstream(input)) {
    PERFETTO_ELOG("Could not parse input.");
    return 1;
  }
  OstreamOutputStream zero_copy_output(output);
  TextFormat::Print(*msg, &zero_copy_output);
  return 0;
}

void PrintFtraceTrack(std::ostream* output,
                      const uint64_t& start,
                      const uint64_t& end,
                      const std::multiset<uint64_t>& ftrace_timestamps) {
  constexpr char kFtraceTrackName[] = "ftrace ";
  size_t width = GetWidth();
  size_t bucket_count = width - strlen(kFtraceTrackName);
  size_t bucket_size = (end - start) / bucket_count;
  size_t max = 0;
  std::vector<size_t> buckets(bucket_count);
  for (size_t i = 0; i < bucket_count; i++) {
    auto low = ftrace_timestamps.lower_bound(i * bucket_size + start);
    auto high = ftrace_timestamps.upper_bound((i + 1) * bucket_size + start);
    buckets[i] = std::distance(low, high);
    max = std::max(max, buckets[i]);
  }

  std::vector<std::string> out =
      std::vector<std::string>({" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇"});
  *output << "-------------------- " << kFtraceTrackName
          << "--------------------\n";
  char line[2048];
  for (size_t i = 0; i < bucket_count; i++) {
    sprintf(
        line, "%s",
        out[std::min(buckets[i] / (max / out.size()), out.size() - 1)].c_str());
    *output << std::string(line);
  }
  *output << "\n\n";
}

void PrintInodeStats(std::ostream* output,
                     const std::set<uint64_t>& inode_numbers,
                     const uint64_t& events_with_inodes) {
  *output << "--------------------Inode Stats-------------------\n";
  char line[2048];
  sprintf(line, "Unique inodes: %zu\n", inode_numbers.size());
  *output << std::string(line);

  sprintf(line, "Events with inodes: %" PRIu64 "\n", events_with_inodes);
  *output << std::string(line);
  *output << "\n";
}

void PrintProcessStats(std::ostream* output,
                       const std::set<pid_t>& tids_in_tree,
                       const std::set<pid_t>& tids_in_events) {
  *output << "----------------Process Tree Stats----------------\n";

  char tid[2048];
  sprintf(tid, "Unique thread ids in process tree: %zu\n", tids_in_tree.size());
  *output << std::string(tid);

  char tid_event[2048];
  sprintf(tid_event, "Unique thread ids in ftrace events: %zu\n",
          tids_in_events.size());
  *output << std::string(tid_event);

  std::set<pid_t> intersect;
  set_intersection(tids_in_tree.begin(), tids_in_tree.end(),
                   tids_in_events.begin(), tids_in_events.end(),
                   std::inserter(intersect, intersect.begin()));

  char matching[2048];
  sprintf(matching, "Thread ids with process info: %zu/%zu -> %zu %%\n\n",
          intersect.size(), tids_in_events.size(),
          (intersect.size() * 100) / tids_in_events.size());
  *output << std::string(matching);
  *output << "\n";
}

int TraceToSummary(std::istream* input, std::ostream* output) {
  uint64_t start = std::numeric_limits<uint64_t>::max();
  uint64_t end = 0;
  std::multiset<uint64_t> ftrace_timestamps;
  std::set<pid_t> tids_in_tree;
  std::set<pid_t> tids_in_events;
  std::set<uint64_t> inode_numbers;
  uint64_t events_with_inodes = 0;

  ForEachPacketInTrace(
      input,
      [&start, &end, &ftrace_timestamps, &tids_in_tree, &tids_in_events,
       &inode_numbers, &events_with_inodes](const protos::TracePacket& packet) {

        if (packet.has_process_tree()) {
          const ProcessTree& tree = packet.process_tree();
          for (Process process : tree.processes()) {
            for (ProcessTree::Thread thread : process.threads()) {
              tids_in_tree.insert(thread.tid());
            }
          }
        }

        if (!packet.has_ftrace_events())
          return;

        const FtraceEventBundle& bundle = packet.ftrace_events();
        uint64_t inode_number = 0;
        for (const FtraceEvent& event : bundle.event()) {
          if (ParseInode(event, &inode_number)) {
            inode_numbers.insert(inode_number);
            events_with_inodes++;
          }
          if (event.pid()) {
            tids_in_events.insert(event.pid());
          }
          if (event.timestamp()) {
            start = std::min<uint64_t>(start, event.timestamp());
            end = std::max<uint64_t>(end, event.timestamp());
            ftrace_timestamps.insert(event.timestamp());
          }
        }
      });

  fprintf(stderr, "\n");

  char line[2048];
  sprintf(line, "Duration: %" PRIu64 "ms\n", (end - start) / (1000 * 1000));
  *output << std::string(line);

  PrintFtraceTrack(output, start, end, ftrace_timestamps);
  PrintProcessStats(output, tids_in_tree, tids_in_events);
  PrintInodeStats(output, inode_numbers, events_with_inodes);

  return 0;
}

}  // namespace
}  // namespace perfetto

namespace {

int Usage(int argc, char** argv) {
  printf("Usage: %s [systrace|text|summary] < trace.proto > trace.txt\n",
         argv[0]);
  return 1;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2)
    return Usage(argc, argv);

  std::string format(argv[1]);

  bool is_systrace = format == "systrace";
  bool is_text = format == "text";
  bool is_summary = format == "summary";

  if (is_systrace)
    return perfetto::TraceToSystrace(&std::cin, &std::cout);
  if (is_text)
    return perfetto::TraceToText(&std::cin, &std::cout);
  if (is_summary)
    return perfetto::TraceToSummary(&std::cin, &std::cout);
  return Usage(argc, argv);
}
