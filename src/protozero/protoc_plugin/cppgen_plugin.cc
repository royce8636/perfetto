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

#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <set>
#include <stack>
#include <vector>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/field_comparator.h>
#include <google/protobuf/util/message_differencer.h>

#include "perfetto/ext/base/string_utils.h"

namespace protozero {
namespace {

using namespace google::protobuf;
using namespace google::protobuf::compiler;
using namespace google::protobuf::io;
using perfetto::base::SplitString;
using perfetto::base::StripChars;
using perfetto::base::StripSuffix;
using perfetto::base::ToUpper;

static constexpr auto TYPE_MESSAGE = FieldDescriptor::TYPE_MESSAGE;

static const char kHeader[] =
    "// DO NOT EDIT. Autogenerated by Perfetto cppgen_plugin\n";

std::string GetProtoHeader(const FileDescriptor* file) {
  return StripSuffix(file->name(), ".proto") + ".pb.h";
}

template <typename T = Descriptor>
std::string GetFullName(const T* msg, bool with_namespace = false) {
  std::string full_type;
  full_type.append(msg->name());
  for (const Descriptor* par = msg->containing_type(); par;
       par = par->containing_type()) {
    full_type.insert(0, par->name() + "_");
  }
  if (with_namespace) {
    std::vector<std::string> namespaces =
        SplitString(msg->file()->package(), ".");
    for (auto it = namespaces.rbegin(); it != namespaces.rend(); it++) {
      full_type.insert(0, *it + "::");
    }
  }
  return full_type;
}

class CppObjGenerator : public ::google::protobuf::compiler::CodeGenerator {
 public:
  CppObjGenerator();
  ~CppObjGenerator() override;

  // CodeGenerator implementation
  bool Generate(const google::protobuf::FileDescriptor* file,
                const std::string& options,
                GeneratorContext* context,
                std::string* error) const override;

 private:
  std::string GetCppType(const FieldDescriptor* field, bool constref) const;
  void GenEnum(const EnumDescriptor*, Printer*) const;
  void GenEnumAliases(const EnumDescriptor*, Printer*) const;
  void GenClassDecl(const Descriptor*, Printer*) const;
  void GenClassDef(const Descriptor*, Printer*) const;
};

CppObjGenerator::CppObjGenerator() = default;
CppObjGenerator::~CppObjGenerator() = default;

bool CppObjGenerator::Generate(const google::protobuf::FileDescriptor* file,
                               const std::string& /*options*/,
                               GeneratorContext* context,
                               std::string* error) const {
  auto get_file_name = [](const FileDescriptor* proto) {
    return StripSuffix(proto->name(), ".proto") + ".gen";
  };

  const std::unique_ptr<ZeroCopyOutputStream> h_fstream(
      context->Open(get_file_name(file) + ".h"));
  const std::unique_ptr<ZeroCopyOutputStream> cc_fstream(
      context->Open(get_file_name(file) + ".cc"));

  // Variables are delimited by $.
  Printer h_printer(h_fstream.get(), '$');
  Printer cc_printer(cc_fstream.get(), '$');

  std::string include_guard = file->package() + "_" + file->name() + "_CPP_H_";
  include_guard = ToUpper(include_guard);
  include_guard = StripChars(include_guard, ".-/\\", '_');

  h_printer.Print(kHeader);
  h_printer.Print("#ifndef $g$\n#define $g$\n\n", "g", include_guard);
  h_printer.Print("#include <stdint.h>\n");
  h_printer.Print("#include <vector>\n");
  h_printer.Print("#include <string>\n");
  h_printer.Print("#include <type_traits>\n\n");
  h_printer.Print("#include \"perfetto/base/copyable_ptr.h\"\n");
  h_printer.Print("#include \"perfetto/base/export.h\"\n\n");

  cc_printer.Print(kHeader);
  cc_printer.Print("#pragma GCC diagnostic push\n");
  cc_printer.Print("#pragma GCC diagnostic ignored \"-Wfloat-equal\"\n");

  // Generate includes for translated types of dependencies.

  // Figure out the subset of imports that are used only for lazy fields. We
  // won't emit a C++ #include for them. This code is overly aggressive at
  // removing imports: it rules them out as soon as it sees one lazy field
  // whose type is defined in that import. A 100% correct solution would require
  // to check that *all* dependent types for a given import are lazy before
  // excluding that. In practice we don't need that because we don't use imports
  // for both lazy and non-lazy fields.
  std::set<std::string> lazy_imports;
  for (int m = 0; m < file->message_type_count(); m++) {
    const Descriptor* msg = file->message_type(m);
    for (int i = 0; i < msg->field_count(); i++) {
      const FieldDescriptor* field = msg->field(i);
      if (field->options().lazy()) {
        lazy_imports.insert(field->message_type()->file()->name());
      }
    }
  }

  // Include the .pb.h for the current file.
  cc_printer.Print("\n#include \"$f$\"\n", "f", GetProtoHeader(file));

  // Recursively traverse all imports and turn them into #include(s).
  std::vector<const FileDescriptor*> imports_to_visit;
  std::set<const FileDescriptor*> imports_visited;
  imports_to_visit.push_back(file);

  while (!imports_to_visit.empty()) {
    const FileDescriptor* cur = imports_to_visit.back();
    imports_to_visit.pop_back();
    imports_visited.insert(cur);
    cc_printer.Print("#include \"$f$.h\"\n", "f", get_file_name(cur));
    for (int i = 0; i < cur->dependency_count(); i++) {
      const FileDescriptor* dep = cur->dependency(i);
      if (imports_visited.count(dep) || lazy_imports.count(dep->name()))
        continue;
      imports_to_visit.push_back(dep);
    }
  }

  // Compute all nested types to generate forward declarations later.

  std::set<const Descriptor*> all_types_seen;  // All deps
  std::set<const EnumDescriptor*> all_enums_seen;

  // We track the types additionally in vectors to guarantee a stable order in
  // the generated output.
  std::vector<const Descriptor*> local_types;  // Cur .proto file only.
  std::vector<const Descriptor*> all_types;    // All deps
  std::vector<const EnumDescriptor*> local_enums;
  std::vector<const EnumDescriptor*> all_enums;

  auto add_enum = [&local_enums, &all_enums, &all_enums_seen,
                   &file](const EnumDescriptor* enum_desc) {
    if (all_enums_seen.count(enum_desc))
      return;
    all_enums_seen.insert(enum_desc);
    all_enums.push_back(enum_desc);
    if (enum_desc->file() == file)
      local_enums.push_back(enum_desc);
  };

  std::stack<const Descriptor*> recursion_stack;
  for (int i = 0; i < file->message_type_count(); i++)
    recursion_stack.push(file->message_type(i));

  while (!recursion_stack.empty()) {
    const Descriptor* msg = recursion_stack.top();
    recursion_stack.pop();
    if (all_types_seen.count(msg))
      continue;
    all_types_seen.insert(msg);
    all_types.push_back(msg);
    if (msg->file() == file)
      local_types.push_back(msg);

    for (int i = 0; i < msg->nested_type_count(); i++)
      recursion_stack.push(msg->nested_type(i));

    for (int i = 0; i < msg->enum_type_count(); i++)
      add_enum(msg->enum_type(i));

    for (int i = 0; i < msg->field_count(); i++) {
      const FieldDescriptor* field = msg->field(i);
      if (field->has_default_value()) {
        *error = "field " + field->name() +
                 ": Explicitly declared default values are not supported";
        return false;
      }
      if (field->options().lazy() &&
          (field->is_repeated() || field->type() != TYPE_MESSAGE)) {
        *error = "[lazy=true] is supported only on non-repeated fields\n";
        return false;
      }

      if (field->type() == TYPE_MESSAGE && !field->options().lazy())
        recursion_stack.push(field->message_type());

      if (field->type() == FieldDescriptor::TYPE_ENUM)
        add_enum(field->enum_type());
    }
  }  //  while (!recursion_stack.empty())

  // Generate forward declarations in the header for proto types.
  h_printer.Print("// Forward declarations for protobuf types.\n");
  std::vector<std::string> namespaces = SplitString(file->package(), ".");
  for (size_t i = 0; i < namespaces.size(); i++)
    h_printer.Print("namespace $n$ {\n", "n", namespaces[i]);

  for (const Descriptor* msg : all_types)
    h_printer.Print("class $n$;\n", "n", GetFullName(msg));

  for (size_t i = 0; i < namespaces.size(); i++)
    h_printer.Print("}\n");

  h_printer.Print("\nnamespace perfetto {\n");
  cc_printer.Print("\nnamespace perfetto {\n");

  // Generate fwd declarations for C++ types.
  for (const EnumDescriptor* enm : all_enums) {
    h_printer.Print("enum $n$ : int;\n", "n", GetFullName(enm));
  }

  for (const Descriptor* msg : all_types)
    h_printer.Print("class $n$;\n", "n", GetFullName(msg));

  // Generate declarations and definitions.
  for (const EnumDescriptor* enm : local_enums)
    GenEnum(enm, &h_printer);

  for (const Descriptor* msg : local_types) {
    GenClassDecl(msg, &h_printer);
    GenClassDef(msg, &cc_printer);
  }

  cc_printer.Print("}  // namespace perfetto\n");
  cc_printer.Print("#pragma GCC diagnostic pop\n");

  h_printer.Print("}  // namespace perfetto\n");
  h_printer.Print("\n#endif  // $g$\n", "g", include_guard);

  return true;
}

std::string CppObjGenerator::GetCppType(const FieldDescriptor* field,
                                        bool constref) const {
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
      assert(!field->options().lazy());
      return constref ? "const " + GetFullName(field->message_type()) + "&"
                      : GetFullName(field->message_type());
    case FieldDescriptor::TYPE_ENUM:
      return GetFullName(field->enum_type());
    case FieldDescriptor::TYPE_GROUP:
      abort();
  }
  abort();  // for gcc
}

void CppObjGenerator::GenEnum(const EnumDescriptor* enum_desc,
                              Printer* p) const {
  std::string full_name = GetFullName(enum_desc);
  p->Print("enum $f$ : int {\n", "f", full_name);
  for (int e = 0; e < enum_desc->value_count(); e++) {
    const EnumValueDescriptor* value = enum_desc->value(e);
    p->Print("  $f$_$n$ = $v$,\n", "f", full_name, "n", value->name(), "v",
             std::to_string(value->number()));
  }
  p->Print("};\n");
}

void CppObjGenerator::GenEnumAliases(const EnumDescriptor* enum_desc,
                                     Printer* p) const {
  std::string full_name = GetFullName(enum_desc);
  for (int e = 0; e < enum_desc->value_count(); e++) {
    const EnumValueDescriptor* value = enum_desc->value(e);
    p->Print("static constexpr auto $n$ = $f$_$n$;\n", "f", full_name, "n",
             value->name());
  }
}

void CppObjGenerator::GenClassDecl(const Descriptor* msg, Printer* p) const {
  std::string full_name = GetFullName(msg);
  p->Print("\nclass PERFETTO_EXPORT $n$ {\n", "n", full_name);
  p->Print(" public:\n");
  p->Indent();

  // Do a first pass to generate aliases for nested types.
  // e.g., using Foo = Parent_Foo;
  for (int i = 0; i < msg->nested_type_count(); i++) {
    const Descriptor* nested_msg = msg->nested_type(i);
    p->Print("using $n$ = $f$;\n", "n", nested_msg->name(), "f",
             GetFullName(nested_msg));
  }
  for (int i = 0; i < msg->enum_type_count(); i++) {
    const EnumDescriptor* nested_enum = msg->enum_type(i);
    p->Print("using $n$ = $f$;\n", "n", nested_enum->name(), "f",
             GetFullName(nested_enum));
    GenEnumAliases(nested_enum, p);
  }

  p->Print("$n$();\n", "n", full_name);
  p->Print("~$n$();\n", "n", full_name);
  p->Print("$n$($n$&&) noexcept;\n", "n", full_name);
  p->Print("$n$& operator=($n$&&);\n", "n", full_name);
  p->Print("$n$(const $n$&);\n", "n", full_name);
  p->Print("$n$& operator=(const $n$&);\n", "n", full_name);
  p->Print("bool operator==(const $n$&) const;\n", "n", full_name);
  p->Print(
      "bool operator!=(const $n$& other) const { return !(*this == other); }\n",
      "n", full_name);
  p->Print("\n");

  std::string proto_type = GetFullName(msg, true);
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
      p->Print("$t$* add_$n$() { $n$_.emplace_back(); return &$n$_.back(); }\n",
               "t", GetCppType(field, false), "n", field->lowercase_name());
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

void CppObjGenerator::GenClassDef(const Descriptor* msg, Printer* p) const {
  p->Print("\n");
  std::string full_name = GetFullName(msg);

  p->Print("$n$::$n$() = default;\n", "n", full_name);
  p->Print("$n$::~$n$() = default;\n", "n", full_name);
  p->Print("$n$::$n$(const $n$&) = default;\n", "n", full_name);
  p->Print("$n$& $n$::operator=(const $n$&) = default;\n", "n", full_name);
  p->Print("$n$::$n$($n$&&) noexcept = default;\n", "n", full_name);
  p->Print("$n$& $n$::operator=($n$&&) = default;\n", "n", full_name);

  p->Print("\n");

  // Comparison operator
  p->Print("bool $n$::operator==(const $n$& other) const {\n", "n", full_name);
  p->Indent();

  p->Print("return unknown_fields_ == other.unknown_fields_");
  for (int i = 0; i < msg->field_count(); i++)
    p->Print("\n && $n$_ == other.$n$_", "n", msg->field(i)->lowercase_name());
  p->Print(";");
  p->Outdent();
  p->Print("\n}\n\n");

  std::string proto_type = GetFullName(msg, true);

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
      p->Print("$n$_ = proto.$n$().SerializeAsString();\n", "n",
               field->lowercase_name());
    } else if (!field->is_repeated()) {
      if (field->type() == TYPE_MESSAGE) {
        p->Print("$n$_->FromProto(proto.$n$());\n", "n",
                 field->lowercase_name());
      } else {
        p->Print(
            "static_assert(sizeof($n$_) == sizeof(proto.$n$()), \"size "
            "mismatch\");\n",
            "n", field->lowercase_name());
        p->Print("$n$_ = static_cast<decltype($n$_)>(proto.$n$());\n", "n",
                 field->lowercase_name());
      }
    } else {  // is_repeated()
      p->Print("$n$_.clear();\n", "n", field->lowercase_name());
      p->Print("for (const auto& field : proto.$n$()) {\n", "n",
               field->lowercase_name());
      p->Print("  $n$_.emplace_back();\n", "n", field->lowercase_name());
      if (field->type() == TYPE_MESSAGE) {
        p->Print("  $n$_.back().FromProto(field);\n", "n",
                 field->lowercase_name());
      } else {
        p->Print(
            "static_assert(sizeof($n$_.back()) == sizeof(proto.$n$(0)), \"size "
            "mismatch\");\n",
            "n", field->lowercase_name());
        p->Print(
            "  $n$_.back() = static_cast<decltype($n$_)::value_type>(field);\n",
            "n", field->lowercase_name());
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
               field->lowercase_name());
    } else if (!field->is_repeated()) {
      if (field->type() == TYPE_MESSAGE) {
        p->Print("$n$_->ToProto(proto->mutable_$n$());\n", "n",
                 field->lowercase_name());
      } else {
        p->Print(
            "static_assert(sizeof($n$_) == sizeof(proto->$n$()), \"size "
            "mismatch\");\n",
            "n", field->lowercase_name());
        p->Print("proto->set_$n$(static_cast<decltype(proto->$n$())>($n$_));\n",
                 "n", field->lowercase_name());
      }
    } else {  // is_repeated()
      p->Print("for (const auto& it : $n$_) {\n", "n", field->lowercase_name());
      if (field->type() == TYPE_MESSAGE) {
        p->Print("  auto* entry = proto->add_$n$();\n", "n",
                 field->lowercase_name());
        p->Print("  it.ToProto(entry);\n");
      } else {
        p->Print(
            "  proto->add_$n$(static_cast<decltype(proto->$n$(0))>(it));\n",
            "n", field->lowercase_name());
        p->Print(
            "static_assert(sizeof(it) == sizeof(proto->$n$(0)), \"size "
            "mismatch\");\n",
            "n", field->lowercase_name());
      }
      p->Print("}\n");
    }
  }
  p->Print("*(proto->mutable_unknown_fields()) = unknown_fields_;\n");
  p->Outdent();
  p->Print("}\n\n");
}

}  // namespace
}  // namespace protozero

int main(int argc, char** argv) {
  ::protozero::CppObjGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
