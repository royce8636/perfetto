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

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/util/field_comparator.h>
#include <google/protobuf/util/message_differencer.h>

#include <stdio.h>

#include <fstream>
#include <iostream>

#include "perfetto/base/logging.h"

using namespace google::protobuf;
using namespace google::protobuf::compiler;
using namespace google::protobuf::io;
static constexpr auto TYPE_MESSAGE = FieldDescriptor::TYPE_MESSAGE;

namespace {

static const char kHeader[] = R"(/*
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

/*******************************************************************************
 * AUTOGENERATED - DO NOT EDIT
 *******************************************************************************
 * This file has been generated from the protobuf message
 * $p$
 * by
 * $f$.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

)";

class ErrorPrinter : public MultiFileErrorCollector {
  virtual void AddError(const string& filename,
                        int line,
                        int col,
                        const string& msg) {
    PERFETTO_ELOG("%s %d:%d: %s", filename.c_str(), line, col, msg.c_str());
  }
  virtual void AddWarning(const string& filename,
                          int line,
                          int col,
                          const string& msg) {
    PERFETTO_ILOG("%s %d:%d: %s", filename.c_str(), line, col, msg.c_str());
  }
};

std::string GetProtoHeader(const FileDescriptor* proto_file) {
  return StringReplace(proto_file->name(), ".proto", ".pb.h", false);
}

std::string GetFwdDeclType(const Descriptor* msg, bool with_namespace = false) {
  std::string full_type;
  full_type.append(msg->name());
  for (const Descriptor* par = msg->containing_type(); par;
       par = par->containing_type()) {
    full_type.insert(0, par->name() + "_");
  }
  if (with_namespace) {
    std::vector<std::string> namespaces = Split(msg->file()->package(), ".");
    for (auto it = namespaces.rbegin(); it != namespaces.rend(); it++) {
      full_type.insert(0, *it + "::");
    }
  }
  return full_type;
}

void GenFwdDecl(const Descriptor* msg, Printer* p, bool root_cpp_only = false) {
  if (!root_cpp_only) {
    p->Print("class $n$;", "n", GetFwdDeclType(msg));
  } else if (msg->full_name() == msg->file()->package() + "." + msg->name()) {
    p->Print("class $n$;", "n", msg->name());
  }

  // Recurse into subtypes
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    if (field->type() == TYPE_MESSAGE && !field->options().lazy()) {
      GenFwdDecl(field->message_type(), p, root_cpp_only);
    }
  }
}

}  // namespace

class ProtoToCpp {
 public:
  ProtoToCpp(const std::string& header_dir,
             const std::string& cpp_dir,
             const std::string& include_path);

  static std::string GetCppType(const FieldDescriptor* field, bool constref);

  void Convert(const std::string& src_proto);
  std::string GetHeaderPath(const FileDescriptor*);
  std::string GetCppPath(const FileDescriptor*);
  std::string GetIncludePath(const FileDescriptor*);
  void GenHeader(const Descriptor*, Printer*);
  void GenCpp(const Descriptor*, Printer*, std::string prefix);

 private:
  std::string header_dir_;
  std::string cpp_dir_;
  std::string include_path_;
  DiskSourceTree dst_;
  ErrorPrinter error_printer_;
  Importer importer_;
};

ProtoToCpp::ProtoToCpp(const std::string& header_dir,
                       const std::string& cpp_dir,
                       const std::string& include_path)
    : header_dir_(header_dir),
      cpp_dir_(cpp_dir),
      include_path_(include_path),
      importer_(&dst_, &error_printer_) {
  dst_.MapPath("", "");  // Yes, this tautology is needed :/.
}

std::string ProtoToCpp::GetHeaderPath(const FileDescriptor* proto_file) {
  std::string basename = Split(proto_file->name(), "/").back();
  return header_dir_ + "/" + StringReplace(basename, ".proto", ".h", false);
}

std::string ProtoToCpp::GetCppPath(const FileDescriptor* proto_file) {
  std::string basename = Split(proto_file->name(), "/").back();
  return cpp_dir_ + "/" + StringReplace(basename, ".proto", ".cc", false);
}

std::string ProtoToCpp::GetIncludePath(const FileDescriptor* proto_file) {
  std::string basename = Split(proto_file->name(), "/").back();
  return include_path_ + "/" + StringReplace(basename, ".proto", ".h", false);
}

std::string ProtoToCpp::GetCppType(const FieldDescriptor* field,
                                   bool constref) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
      return "double";
    case FieldDescriptor::TYPE_FLOAT:
      return "float";
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
      return "uint32_t";
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
      return "int32_t";
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
      return "uint64_t";
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_INT64:
      return "int64_t";
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return constref ? "const std::string&" : "std::string";
    case FieldDescriptor::TYPE_MESSAGE:
      PERFETTO_CHECK(!field->options().lazy());
      return constref ? "const " + field->message_type()->name() + "&"
                      : field->message_type()->name();
    case FieldDescriptor::TYPE_ENUM:
      return field->enum_type()->name();
    case FieldDescriptor::TYPE_GROUP:
      PERFETTO_FATAL("No cpp type for a group field.");
  }
  PERFETTO_FATAL("Not reached");  // for gcc
}

void ProtoToCpp::Convert(const std::string& src_proto) {
  const FileDescriptor* proto_file = importer_.Import(src_proto);
  if (!proto_file) {
    PERFETTO_ELOG("Failed to load %s", src_proto.c_str());
    exit(1);
  }

  std::string dst_header = GetHeaderPath(proto_file);
  std::string dst_cpp = GetCppPath(proto_file);

  std::ofstream header_ostr;
  header_ostr.open(dst_header);
  PERFETTO_CHECK(header_ostr.is_open());
  OstreamOutputStream header_proto_ostr(&header_ostr);
  Printer header_printer(&header_proto_ostr, '$');

  std::ofstream cpp_ostr;
  cpp_ostr.open(dst_cpp);
  PERFETTO_CHECK(cpp_ostr.is_open());
  OstreamOutputStream cpp_proto_ostr(&cpp_ostr);
  Printer cpp_printer(&cpp_proto_ostr, '$');

  std::string include_guard = dst_header + "_";
  UpperString(&include_guard);
  StripString(&include_guard, ".-/\\", '_');
  header_printer.Print(kHeader, "f", __FILE__, "p", src_proto);
  header_printer.Print("#ifndef $g$\n#define $g$\n\n", "g", include_guard);
  header_printer.Print("#include <stdint.h>\n");
  header_printer.Print("#include <vector>\n");
  header_printer.Print("#include <string>\n");
  header_printer.Print("#include <type_traits>\n\n");
  header_printer.Print("#include \"perfetto/base/copyable_ptr.h\"\n");
  header_printer.Print("#include \"perfetto/base/export.h\"\n\n");

  cpp_printer.Print(kHeader, "f", __FILE__, "p", src_proto);
  if (dst_header.find("include/") == 0) {
    cpp_printer.Print("#include \"$f$\"\n", "f",
                      dst_header.substr(strlen("include/")));
  } else {
    cpp_printer.Print("#include \"$f$\"\n", "f", dst_header);
  }

  // Generate includes for translated types of dependencies.

  // Figure out the subset of imports that are used only for lazy fields. We
  // won't emit a C++ #include for them. This code is overly aggressive at
  // removing imports: it rules them out as soon as it sees one lazy field
  // whose type is defined in that import. A 100% correct solution would require
  // to check that *all* dependent types for a given import are lazy before
  // excluding that. In practice we don't need that because we don't use imports
  // for both lazy and non-lazy fields.
  std::set<std::string> lazy_imports;
  for (int m = 0; m < proto_file->message_type_count(); m++) {
    const Descriptor* msg = proto_file->message_type(m);
    for (int i = 0; i < msg->field_count(); i++) {
      const FieldDescriptor* field = msg->field(i);
      if (field->options().lazy()) {
        lazy_imports.insert(field->message_type()->file()->name());
      }
    }
  }

  cpp_printer.Print("\n#include \"$f$\"\n", "f", GetProtoHeader(proto_file));
  for (int i = 0; i < proto_file->dependency_count(); i++) {
    const FileDescriptor* dep = proto_file->dependency(i);
    if (lazy_imports.count(dep->name()))
      continue;
    cpp_printer.Print("\n#include \"$f$\"\n", "f", GetIncludePath(dep));
    cpp_printer.Print("#include \"$f$\"\n", "f", GetProtoHeader(dep));
  }

  // Generate forward declarations in the header for proto types.
  header_printer.Print("// Forward declarations for protobuf types.\n");
  std::vector<std::string> namespaces = Split(proto_file->package(), ".");
  for (size_t i = 0; i < namespaces.size(); i++)
    header_printer.Print("namespace $n$ {\n", "n", namespaces[i]);
  for (int i = 0; i < proto_file->message_type_count(); i++)
    GenFwdDecl(proto_file->message_type(i), &header_printer);
  for (size_t i = 0; i < namespaces.size(); i++)
    header_printer.Print("}\n");

  header_printer.Print("\nnamespace perfetto {\n");
  cpp_printer.Print("\nnamespace perfetto {\n");

  // Generate fwd declarations for top-level classes.
  for (int i = 0; i < proto_file->message_type_count(); i++)
    GenFwdDecl(proto_file->message_type(i), &header_printer, true);
  header_printer.Print("\n");

  for (int i = 0; i < proto_file->message_type_count(); i++) {
    const Descriptor* msg = proto_file->message_type(i);
    GenHeader(msg, &header_printer);
    GenCpp(msg, &cpp_printer, "");
  }

  cpp_printer.Print("}  // namespace perfetto\n");
  header_printer.Print("}  // namespace perfetto\n");
  header_printer.Print("\n#endif  // $g$\n", "g", include_guard);
}

void ProtoToCpp::GenHeader(const Descriptor* msg, Printer* p) {
  PERFETTO_ILOG("GEN %s %s", msg->name().c_str(), msg->file()->name().c_str());
  p->Print("\nclass PERFETTO_EXPORT $n$ {\n", "n", msg->name());
  p->Print(" public:\n");
  p->Indent();
  // Do a first pass to generate nested types.
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    if (field->has_default_value()) {
      PERFETTO_FATAL(
          "Error on field %s: Explicitly declared default values are not "
          "supported",
          field->name().c_str());
    }

    if (field->type() == FieldDescriptor::TYPE_ENUM) {
      const EnumDescriptor* enum_desc = field->enum_type();
      p->Print("enum $n$ {\n", "n", enum_desc->name());
      for (int e = 0; e < enum_desc->value_count(); e++) {
        const EnumValueDescriptor* value = enum_desc->value(e);
        p->Print("  $n$ = $v$,\n", "n", value->name(), "v",
                 std::to_string(value->number()));
      }
      p->Print("};\n");
    } else if (field->type() == TYPE_MESSAGE &&
               field->message_type()->file() == msg->file()) {
      GenHeader(field->message_type(), p);
    }
  }

  p->Print("$n$();\n", "n", msg->name());
  p->Print("~$n$();\n", "n", msg->name());
  p->Print("$n$($n$&&) noexcept;\n", "n", msg->name());
  p->Print("$n$& operator=($n$&&);\n", "n", msg->name());
  p->Print("$n$(const $n$&);\n", "n", msg->name());
  p->Print("$n$& operator=(const $n$&);\n", "n", msg->name());
  p->Print("bool operator==(const $n$&) const;\n", "n", msg->name());
  p->Print(
      "bool operator!=(const $n$& other) const { return !(*this == other); }\n",
      "n", msg->name());
  p->Print("\n");

  std::string proto_type = GetFwdDeclType(msg, true);
  p->Print("// Raw proto decoding.\n");
  p->Print("void ParseRawProto(const std::string&);\n");
  p->Print("// Conversion methods from/to the corresponding protobuf types.\n");
  p->Print("void FromProto(const $p$&);\n", "p", proto_type);
  p->Print("void ToProto($p$*) const;\n", "p", proto_type);

  // Generate accessors.
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    p->Print("\n");
    if (field->options().lazy()) {
      if (field->is_repeated() || field->type() != TYPE_MESSAGE) {
        PERFETTO_FATAL(
            "[lazy=true] is supported only on non-repeated submessage fields");
      }
      p->Print("const std::string& $n$_raw() const { return $n$_; }\n", "n",
               field->lowercase_name());
      p->Print("void set_$n$_raw(const std::string& raw) { $n$_ = raw; }\n",
               "n", field->lowercase_name());
    } else if (!field->is_repeated()) {
      if (field->type() == TYPE_MESSAGE) {
        p->Print("$t$ $n$() const { return *$n$_; }\n", "t",
                 GetCppType(field, true), "n", field->lowercase_name());
        p->Print("$t$* mutable_$n$() { return $n$_.get(); }\n", "t",
                 GetCppType(field, false), "n", field->lowercase_name());
      } else {
        p->Print("$t$ $n$() const { return $n$_; }\n", "t",
                 GetCppType(field, true), "n", field->lowercase_name());
        p->Print("void set_$n$($t$ value) { $n$_ = value; }\n", "t",
                 GetCppType(field, true), "n", field->lowercase_name());
        if (field->type() == FieldDescriptor::TYPE_BYTES) {
          p->Print(
              "void set_$n$(const void* p, size_t s) { "
              "$n$_.assign(reinterpret_cast<const char*>(p), s); }\n",
              "n", field->lowercase_name());
        }
      }
    } else {  // is_repeated()
      p->Print(
          "int $n$_size() const { return static_cast<int>($n$_.size()); }\n",
          "t", GetCppType(field, false), "n", field->lowercase_name());
      p->Print("const std::vector<$t$>& $n$() const { return $n$_; }\n", "t",
               GetCppType(field, false), "n", field->lowercase_name());
      p->Print("std::vector<$t$>* mutable_$n$() { return &$n$_; }\n", "t",
               GetCppType(field, false), "n", field->lowercase_name());
      p->Print("void clear_$n$() { $n$_.clear(); }\n", "n",
               field->lowercase_name());

      if (field->type() == TYPE_MESSAGE && !field->options().lazy()) {
        p->Print(
            "$t$* add_$n$() { $n$_.emplace_back(); return &$n$_.back(); "
            "}\n",
            "t", GetCppType(field, false), "n", field->lowercase_name());
      } else {
        p->Print(
            "$t$* add_$n$() { $n$_.emplace_back(); return &$n$_.back(); }\n",
            "t", GetCppType(field, false), "n", field->lowercase_name());
      }
    }
  }
  p->Outdent();
  p->Print("\n private:\n");
  p->Indent();

  // Generate fields.
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    if (field->options().lazy()) {
      p->Print("std::string $n$_;  // [lazy=true]\n", "n",
               field->lowercase_name());
    } else if (!field->is_repeated()) {
      std::string type = GetCppType(field, false);
      if (field->type() == TYPE_MESSAGE) {
        type = "::perfetto::base::CopyablePtr<" + type + ">";
        p->Print("$t$ $n$_;\n", "t", type, "n", field->lowercase_name());
      } else {
        p->Print("$t$ $n$_{};\n", "t", type, "n", field->lowercase_name());
      }
    } else {  // is_repeated()
      p->Print("std::vector<$t$> $n$_;\n", "t", GetCppType(field, false), "n",
               field->lowercase_name());
    }
  }
  p->Print("\n");
  p->Print("// Allows to preserve unknown protobuf fields for compatibility\n");
  p->Print("// with future versions of .proto files.\n");
  p->Print("std::string unknown_fields_;\n");
  p->Outdent();
  p->Print("};\n\n");
}

void ProtoToCpp::GenCpp(const Descriptor* msg, Printer* p, std::string prefix) {
  p->Print("\n");
  std::string full_name = prefix + msg->name();

  p->Print("$f$::$n$() = default;\n", "f", full_name, "n", msg->name());
  p->Print("$f$::~$n$() = default;\n", "f", full_name, "n", msg->name());
  p->Print("$f$::$n$(const $f$&) = default;\n", "f", full_name, "n",
           msg->name());
  p->Print("$f$& $f$::operator=(const $f$&) = default;\n", "f", full_name, "n",
           msg->name());
  p->Print("$f$::$n$($f$&&) noexcept = default;\n", "f", full_name, "n",
           msg->name());
  p->Print("$f$& $f$::operator=($f$&&) = default;\n", "f", full_name, "n",
           msg->name());

  p->Print("\n");

  // Comparison operator
  p->Print("#pragma GCC diagnostic push\n");
  p->Print("#pragma GCC diagnostic ignored \"-Wfloat-equal\"\n");
  p->Print("bool $f$::operator==(const $f$& other) const {\n", "f", full_name,
           "n", msg->name());
  p->Indent();

  p->Print("return ");
  for (int i = 0; i < msg->field_count(); i++) {
    if (i > 0)
      p->Print("\n && ");
    const FieldDescriptor* field = msg->field(i);
    p->Print("($n$_ == other.$n$_)", "n", field->name());
  }
  p->Print(";");
  p->Outdent();
  p->Print("}\n");
  p->Print("#pragma GCC diagnostic pop\n\n");

  std::string proto_type = GetFwdDeclType(msg, true);

  // Genrate the ParseRawProto() method definition.
  p->Print("void $f$::ParseRawProto(const std::string& raw) {\n", "f",
           full_name);
  p->Indent();
  p->Print("$p$ proto;\n", "p", proto_type);
  p->Print("proto.ParseFromString(raw);\n");
  p->Print("FromProto(proto);\n");
  p->Outdent();
  p->Print("}\n\n");

  // Genrate the FromProto() method definition.
  p->Print("void $f$::FromProto(const $p$& proto) {\n", "f", full_name, "p",
           proto_type);
  p->Indent();
  for (int i = 0; i < msg->field_count(); i++) {
    p->Print("\n");
    const FieldDescriptor* field = msg->field(i);
    if (field->options().lazy()) {
      p->Print("$n$_ = proto.$n$().SerializeAsString();\n", "n", field->name());
    } else if (!field->is_repeated()) {
      if (field->type() == TYPE_MESSAGE) {
        p->Print("$n$_->FromProto(proto.$n$());\n", "n", field->name());
      } else {
        p->Print(
            "static_assert(sizeof($n$_) == sizeof(proto.$n$()), \"size "
            "mismatch\");\n",
            "n", field->name());
        p->Print("$n$_ = static_cast<decltype($n$_)>(proto.$n$());\n", "n",
                 field->name());
      }
    } else {  // is_repeated()
      p->Print("$n$_.clear();\n", "n", field->name());
      p->Print("for (const auto& field : proto.$n$()) {\n", "n", field->name());
      p->Print("  $n$_.emplace_back();\n", "n", field->name());
      if (field->type() == TYPE_MESSAGE) {
        p->Print("  $n$_.back().FromProto(field);\n", "n", field->name());
      } else {
        p->Print(
            "static_assert(sizeof($n$_.back()) == sizeof(proto.$n$(0)), \"size "
            "mismatch\");\n",
            "n", field->name());
        p->Print(
            "  $n$_.back() = static_cast<decltype($n$_)::value_type>(field);\n",
            "n", field->name());
      }
      p->Print("}\n");
    }
  }
  p->Print("unknown_fields_ = proto.unknown_fields();\n");
  p->Outdent();
  p->Print("}\n\n");

  // Genrate the ToProto() method definition.
  p->Print("void $f$::ToProto($p$* proto) const {\n", "f", full_name, "p",
           proto_type);
  p->Indent();
  p->Print("proto->Clear();\n");
  for (int i = 0; i < msg->field_count(); i++) {
    p->Print("\n");
    const FieldDescriptor* field = msg->field(i);
    if (field->options().lazy()) {
      p->Print("proto->mutable_$n$()->ParseFromString($n$_);\n", "n",
               field->name());
    } else if (!field->is_repeated()) {
      if (field->type() == TYPE_MESSAGE) {
        p->Print("$n$_->ToProto(proto->mutable_$n$());\n", "n", field->name());
      } else {
        p->Print(
            "static_assert(sizeof($n$_) == sizeof(proto->$n$()), \"size "
            "mismatch\");\n",
            "n", field->name());
        p->Print("proto->set_$n$(static_cast<decltype(proto->$n$())>($n$_));\n",
                 "n", field->name());
      }
    } else {  // is_repeated()
      p->Print("for (const auto& it : $n$_) {\n", "n", field->name());
      if (field->type() == TYPE_MESSAGE) {
        p->Print("  auto* entry = proto->add_$n$();\n", "n", field->name());
        p->Print("  it.ToProto(entry);\n");
      } else {
        p->Print(
            "  proto->add_$n$(static_cast<decltype(proto->$n$(0))>(it));\n",
            "n", field->name());
        p->Print(
            "static_assert(sizeof(it) == sizeof(proto->$n$(0)), \"size "
            "mismatch\");\n",
            "n", field->name());
      }
      p->Print("}\n");
    }
  }
  p->Print("*(proto->mutable_unknown_fields()) = unknown_fields_;\n");
  p->Outdent();
  p->Print("}\n\n");

  // Recurse into nested types.
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    // [lazy=true] field are not recursesd and exposed as raw-strings
    // containing proto-encoded bytes.
    if (field->options().lazy())
      continue;

    if (field->type() == TYPE_MESSAGE &&
        field->message_type()->file() == msg->file()) {
      std::string child_prefix = prefix + msg->name() + "::";
      GenCpp(field->message_type(), p, child_prefix);
    }
  }
}

int main(int argc, char** argv) {
  if (argc <= 4) {
    PERFETTO_ELOG(
        "Usage: %s source.proto dir/for/header dir/for/cc include/path",
        argv[0]);
    return 1;
  }
  ProtoToCpp proto_to_cpp(argv[2], argv[3], argv[4]);
  proto_to_cpp.Convert(argv[1]);
  return 0;
}
