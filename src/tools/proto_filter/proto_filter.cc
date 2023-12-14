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

#include "perfetto/ext/base/file_utils.h"
#include "perfetto/ext/base/getopt.h"
#include "perfetto/ext/base/scoped_file.h"
#include "perfetto/ext/base/string_utils.h"
#include "perfetto/ext/base/version.h"
#include "protos/perfetto/config/trace_config.gen.h"
#include "src/perfetto_cmd/pbtxt_to_pb.h"
#include "src/protozero/filtering/filter_util.h"
#include "src/protozero/filtering/message_filter.h"

namespace perfetto {
namespace proto_filter {
namespace {

const char kUsage[] =
    R"(Usage: proto_filter [options]

-s --schema-in:      Path to the root .proto file. Required for most operations
-I --proto_path:     Extra include directory for proto includes. If omitted assumed CWD.
-r --root_message:   Fully qualified name for the root proto message (e.g. perfetto.protos.Trace)
                     If omitted the first message defined in the schema will be used.
-i --msg_in:         Path of a binary-encoded proto message which will be filtered.
-o --msg_out:        Path of the binary-encoded filtered proto message written in output.
-c --config_in:      Path of a TraceConfig textproto (note: only trace_filter field is considered).
-f --filter_in:      Path of a filter bytecode file previously generated by this tool.
-F --filter_out:     Path of the filter bytecode file generated from the --schema-in definition.
-T --filter_oct_out: Like --filter_out, but emits a octal-escaped C string suitable for .pbtx.
-d --dedupe:         Minimize filter size by deduping leaf messages with same field ids.
-x --passthrough:    Passthrough a nested message as an opaque bytes field.
-g --filter_string:  Filter the string using separately specified rules before passing it through.

Example usage:

# Convert a .proto schema file into a diff-friendly list of messages/fields>

  proto_filter -r perfetto.protos.Trace -s protos/perfetto/trace/trace.proto

# Generate the filter bytecode from a .proto schema

  proto_filter -r perfetto.protos.Trace -s protos/perfetto/trace/trace.proto \
               -F /tmp/bytecode [--dedupe] \
               [-x protos.Message:message_field_to_pass] \
               [-r protos.Message:string_field_to_filter]

# List the used/filtered fields from a trace file

  proto_filter -r perfetto.protos.Trace -s protos/perfetto/trace/trace.proto \
               -i test/data/example_android_trace_30s.pb -f /tmp/bytecode

# Filter a trace using a filter bytecode

  proto_filter -i test/data/example_android_trace_30s.pb -f /tmp/bytecode \
               -o /tmp/filtered_trace

# Filter a trace using a TraceConfig textproto

  proto_filter -i test/data/example_android_trace_30s.pb \
               -c /tmp/config.textproto \
               -o /tmp/filtered_trace

# Show which fields are allowed by a filter bytecode

  proto_filter -r perfetto.protos.Trace -s protos/perfetto/trace/trace.proto \
               -f /tmp/bytecode
)";

class LoggingErrorReporter : public ErrorReporter {
 public:
  LoggingErrorReporter(std::string file_name, const char* config)
      : file_name_(file_name), config_(config) {}

  void AddError(size_t row,
                size_t column,
                size_t length,
                const std::string& message) override {
    parsed_successfully_ = false;
    std::string line = ExtractLine(row - 1).ToStdString();
    if (!line.empty() && line[line.length() - 1] == '\n') {
      line.erase(line.length() - 1);
    }

    std::string guide(column + length, ' ');
    for (size_t i = column; i < column + length; i++) {
      guide[i - 1] = i == column ? '^' : '~';
    }
    fprintf(stderr, "%s:%zu:%zu error: %s\n", file_name_.c_str(), row, column,
            message.c_str());
    fprintf(stderr, "%s\n", line.c_str());
    fprintf(stderr, "%s\n", guide.c_str());
  }

  bool Success() const { return parsed_successfully_; }

 private:
  base::StringView ExtractLine(size_t line) {
    const char* start = config_;
    const char* end = config_;

    for (size_t i = 0; i < line + 1; i++) {
      start = end;
      char c;
      while ((c = *end++) && c != '\n')
        ;
    }
    return base::StringView(start, static_cast<size_t>(end - start));
  }

  bool parsed_successfully_ = true;
  std::string file_name_;
  const char* config_;
};

using TraceFilter = protos::gen::TraceConfig::TraceFilter;
std::optional<protozero::StringFilter::Policy> ConvertPolicy(
    TraceFilter::StringFilterPolicy policy) {
  switch (policy) {
    case TraceFilter::SFP_UNSPECIFIED:
      return std::nullopt;
    case TraceFilter::SFP_MATCH_REDACT_GROUPS:
      return protozero::StringFilter::Policy::kMatchRedactGroups;
    case TraceFilter::SFP_ATRACE_MATCH_REDACT_GROUPS:
      return protozero::StringFilter::Policy::kAtraceMatchRedactGroups;
    case TraceFilter::SFP_MATCH_BREAK:
      return protozero::StringFilter::Policy::kMatchBreak;
    case TraceFilter::SFP_ATRACE_MATCH_BREAK:
      return protozero::StringFilter::Policy::kAtraceMatchBreak;
    case TraceFilter::SFP_ATRACE_REPEATED_SEARCH_REDACT_GROUPS:
      return protozero::StringFilter::Policy::kAtraceRepeatedSearchRedactGroups;
  }
  return std::nullopt;
}

int Main(int argc, char** argv) {
  static const option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"version", no_argument, nullptr, 'v'},
      {"dedupe", no_argument, nullptr, 'd'},
      {"proto_path", required_argument, nullptr, 'I'},
      {"schema_in", required_argument, nullptr, 's'},
      {"root_message", required_argument, nullptr, 'r'},
      {"msg_in", required_argument, nullptr, 'i'},
      {"msg_out", required_argument, nullptr, 'o'},
      {"config_in", required_argument, nullptr, 'c'},
      {"filter_in", required_argument, nullptr, 'f'},
      {"filter_out", required_argument, nullptr, 'F'},
      {"filter_oct_out", required_argument, nullptr, 'T'},
      {"passthrough", required_argument, nullptr, 'x'},
      {"filter_string", required_argument, nullptr, 'g'},
      {nullptr, 0, nullptr, 0}};

  std::string msg_in;
  std::string msg_out;
  std::string config_in;
  std::string filter_in;
  std::string schema_in;
  std::string filter_out;
  std::string filter_oct_out;
  std::string proto_path;
  std::string root_message_arg;
  std::set<std::string> passthrough_fields;
  std::set<std::string> filter_string_fields;
  bool dedupe = false;

  for (;;) {
    int option = getopt_long(
        argc, argv, "hvdI:s:r:i:o:f:F:T:x:g:c:", long_options, nullptr);

    if (option == -1)
      break;  // EOF.

    if (option == 'v') {
      printf("%s\n", base::GetVersionString());
      exit(0);
    }

    if (option == 'd') {
      dedupe = true;
      continue;
    }

    if (option == 'I') {
      proto_path = optarg;
      continue;
    }

    if (option == 's') {
      schema_in = optarg;
      continue;
    }

    if (option == 'c') {
      config_in = optarg;
      continue;
    }

    if (option == 'r') {
      root_message_arg = optarg;
      continue;
    }

    if (option == 'i') {
      msg_in = optarg;
      continue;
    }

    if (option == 'o') {
      msg_out = optarg;
      continue;
    }

    if (option == 'f') {
      filter_in = optarg;
      continue;
    }

    if (option == 'F') {
      filter_out = optarg;
      continue;
    }

    if (option == 'T') {
      filter_oct_out = optarg;
      continue;
    }

    if (option == 'x') {
      passthrough_fields.insert(optarg);
      continue;
    }

    if (option == 'g') {
      filter_string_fields.insert(optarg);
      continue;
    }

    if (option == 'h') {
      fprintf(stdout, kUsage);
      exit(0);
    }

    fprintf(stderr, kUsage);
    exit(1);
  }

  if (msg_in.empty() && filter_in.empty() && schema_in.empty()) {
    fprintf(stderr, kUsage);
    return 1;
  }

  if (!filter_in.empty() && !config_in.empty()) {
    fprintf(stderr, kUsage);
    return 1;
  }

  std::string msg_in_data;
  if (!msg_in.empty()) {
    PERFETTO_LOG("Loading proto-encoded message from %s", msg_in.c_str());
    if (!base::ReadFile(msg_in, &msg_in_data)) {
      PERFETTO_ELOG("Could not open message file %s", msg_in.c_str());
      return 1;
    }
  }

  protozero::FilterUtil filter;
  if (!schema_in.empty()) {
    PERFETTO_LOG("Loading proto schema from %s", schema_in.c_str());
    if (!filter.LoadMessageDefinition(schema_in, root_message_arg, proto_path,
                                      passthrough_fields,
                                      filter_string_fields)) {
      PERFETTO_ELOG("Failed to parse proto schema from %s", schema_in.c_str());
      return 1;
    }
    if (dedupe)
      filter.Dedupe();
  }

  protozero::MessageFilter msg_filter;
  std::string filter_data;
  std::string filter_data_src;
  if (!filter_in.empty()) {
    PERFETTO_LOG("Loading filter bytecode from %s", filter_in.c_str());
    if (!base::ReadFile(filter_in, &filter_data)) {
      PERFETTO_ELOG("Could not open filter file %s", filter_in.c_str());
      return 1;
    }
    filter_data_src = filter_in;
  } else if (!config_in.empty()) {
    PERFETTO_LOG("Loading filter bytecode and rules from %s",
                 config_in.c_str());
    std::string config_data;
    if (!base::ReadFile(config_in, &config_data)) {
      PERFETTO_ELOG("Could not open config file %s", config_in.c_str());
      return 1;
    }
    LoggingErrorReporter reporter(config_in, config_data.c_str());
    auto config_bytes = PbtxtToPb(config_data, &reporter);
    if (!reporter.Success()) {
      return 1;
    }

    protos::gen::TraceConfig config;
    config.ParseFromArray(config_bytes.data(), config_bytes.size());

    const auto& trace_filter = config.trace_filter();
    for (const auto& rule : trace_filter.string_filter_chain().rules()) {
      auto opt_policy = ConvertPolicy(rule.policy());
      if (!opt_policy) {
        PERFETTO_ELOG("Unknown string filter policy %d", rule.policy());
        return 1;
      }
      msg_filter.string_filter().AddRule(*opt_policy, rule.regex_pattern(),
                                         rule.atrace_payload_starts_with());
    }
    filter_data = trace_filter.bytecode_v2().empty()
                      ? trace_filter.bytecode()
                      : trace_filter.bytecode_v2();
    filter_data_src = config_in;
  } else if (!schema_in.empty()) {
    PERFETTO_LOG("Generating filter bytecode from %s", schema_in.c_str());
    filter_data = filter.GenerateFilterBytecode();
    filter_data_src = schema_in;
  }

  if (!filter_data.empty()) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(filter_data.data());
    if (!msg_filter.LoadFilterBytecode(data, filter_data.size())) {
      PERFETTO_ELOG("Failed to parse filter bytecode from %s",
                    filter_data_src.c_str());
      return 1;
    }
  }

  // Write the filter bytecode in output.
  if (!filter_out.empty()) {
    auto fd = base::OpenFile(filter_out, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (!fd) {
      PERFETTO_ELOG("Could not open filter out path %s", filter_out.c_str());
      return 1;
    }
    PERFETTO_LOG("Writing filter bytecode (%zu bytes) into %s",
                 filter_data.size(), filter_out.c_str());
    base::WriteAll(*fd, filter_data.data(), filter_data.size());
  }

  if (!filter_oct_out.empty()) {
    auto fd =
        base::OpenFile(filter_oct_out, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (!fd) {
      PERFETTO_ELOG("Could not open filter out path %s",
                    filter_oct_out.c_str());
      return 1;
    }
    std::string oct_str;
    oct_str.reserve(filter_data.size() * 4 + 64);
    oct_str.append("trace_filter {\n  bytecode: \"");
    for (char c : filter_data) {
      uint8_t octect = static_cast<uint8_t>(c);
      char buf[5]{'\\', '0', '0', '0', 0};
      for (uint8_t i = 0; i < 3; ++i) {
        buf[3 - i] = static_cast<char>('0' + static_cast<uint8_t>(octect) % 8);
        octect /= 8;
      }
      oct_str.append(buf);
    }
    oct_str.append("\"\n}\n");
    PERFETTO_LOG("Writing filter bytecode (%zu bytes) into %s", oct_str.size(),
                 filter_oct_out.c_str());
    base::WriteAll(*fd, oct_str.data(), oct_str.size());
  }

  // Apply the filter to the input message (if any).
  std::vector<uint8_t> msg_filtered_data;
  if (!msg_in.empty()) {
    PERFETTO_LOG("Applying filter %s to proto message %s",
                 filter_data_src.c_str(), msg_in.c_str());
    msg_filter.enable_field_usage_tracking(true);
    auto res = msg_filter.FilterMessage(msg_in_data.data(), msg_in_data.size());
    if (res.error)
      PERFETTO_FATAL("Filtering failed");
    msg_filtered_data.insert(msg_filtered_data.end(), res.data.get(),
                             res.data.get() + res.size);
  }

  // Write out the filtered message.
  if (!msg_out.empty()) {
    PERFETTO_LOG("Writing filtered proto bytes (%zu bytes) into %s",
                 msg_filtered_data.size(), msg_out.c_str());
    auto fd = base::OpenFile(msg_out, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    base::WriteAll(*fd, msg_filtered_data.data(), msg_filtered_data.size());
  }

  if (!msg_in.empty()) {
    const auto& field_usage_map = msg_filter.field_usage();
    for (const auto& it : field_usage_map) {
      const std::string& field_path_varint = it.first;
      int32_t num_occurrences = it.second;
      std::string path_str = filter.LookupField(field_path_varint);
      printf("%-100s %s %d\n", path_str.c_str(),
             num_occurrences < 0 ? "DROP" : "PASS", std::abs(num_occurrences));
    }
  } else if (!schema_in.empty()) {
    filter.PrintAsText(!filter_data.empty() ? std::make_optional(filter_data)
                                            : std::nullopt);
  }

  if ((!filter_out.empty() || !filter_oct_out.empty()) && !dedupe) {
    PERFETTO_ELOG(
        "Warning: looks like you are generating a filter without --dedupe. For "
        "production use cases, --dedupe can make the output bytecode "
        "significantly smaller.");
  }
  return 0;
}

}  // namespace
}  // namespace proto_filter
}  // namespace perfetto

int main(int argc, char** argv) {
  return perfetto::proto_filter::Main(argc, argv);
}
