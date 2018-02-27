/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <sys/stat.h>
#include <fstream>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <string>

#include "ftrace_proto_gen.h"
#include "perfetto/ftrace_reader/format_parser.h"
#include "perfetto/trace/ftrace/ftrace_event.pbzero.h"

int main(int argc, const char** argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: ./%s whitelist_dir input_dir output_dir\n",
            argv[0]);
    return 1;
  }

  const char* whitelist_path = argv[1];
  const char* input_dir = argv[2];
  const char* output_dir = argv[3];

  std::set<std::string> events = perfetto::GetWhitelistedEvents(whitelist_path);
  std::vector<std::string> events_info;

  // proto_field_id for each event is read from this file.
  std::ifstream input("protos/perfetto/trace/ftrace/ftrace_event.proto",
                      std::ios::in | std::ios::binary);
  if (!input) {
    fprintf(stderr, "Failed to open %s\n",
            "protos/perfetto/trace/ftrace/ftrace_event.proto");
    return 1;
  }
  std::ostringstream ftrace_stream;
  ftrace_stream << input.rdbuf();

  std::set<std::string> new_events;
  for (auto event : events) {
    std::string file_name =
        event.substr(event.find('/') + 1, std::string::npos);
    struct stat buf;
    if (stat(("protos/perfetto/trace/ftrace/" + file_name + ".proto").c_str(),
             &buf) == -1) {
      new_events.insert(file_name);
    }
  }

  if (!new_events.empty()) {
    perfetto::PrintFtraceEventProtoAdditions(new_events);
    perfetto::PrintTraceToTextMain(new_events);
    perfetto::PrintTraceToTextUsingStatements(new_events);
    perfetto::PrintTraceToTextFunctions(new_events);
  }

  for (auto event : events) {
    std::string proto_file_name =
        event.substr(event.find('/') + 1, std::string::npos) + ".proto";
    std::string group = event.substr(0, event.find('/'));
    std::string input_path = input_dir + event + std::string("/format");
    std::string output_path = output_dir + std::string("/") + proto_file_name;

    std::ifstream fin(input_path.c_str(), std::ios::in);
    if (!fin) {
      fprintf(stderr, "Failed to open %s\n", input_path.c_str());
      return 1;
    }
    std::ostringstream stream;
    stream << fin.rdbuf();
    fin.close();
    std::string contents = stream.str();

    perfetto::FtraceEvent format;
    if (!perfetto::ParseFtraceEvent(contents, &format)) {
      fprintf(stderr, "Could not parse file %s.\n", input_path.c_str());
      return 1;
    }

    perfetto::Proto proto;
    if (!perfetto::GenerateProto(format, &proto)) {
      fprintf(stderr, "Could not generate proto for file %s\n",
              input_path.c_str());
      return 1;
    }

    std::smatch match;
    std::string ftrace =
        ftrace_stream.str();  // regex_search requires a non-temporary string
    std::regex event_regex(format.name + "\\s*=\\s*(\\d+)");
    std::regex_search(ftrace, match, event_regex);
    std::string proto_field_id = match[1].str().c_str();
    if (proto_field_id == "") {
      fprintf(stderr,
              "Could not find proto_field_id for %s in ftrace_event.proto. "
              "Please add it.\n",
              format.name.c_str());
      return 1;
    }

    events_info.push_back(
        perfetto::SingleEventInfo(format, proto, group, proto_field_id));

    std::ofstream fout(output_path.c_str(), std::ios::out);
    if (!fout) {
      fprintf(stderr, "Failed to open %s\n", output_path.c_str());
      return 1;
    }

    fout << proto.ToString();
    fout.close();
  }

  input.close();
  perfetto::GenerateEventInfo(events_info);
}
