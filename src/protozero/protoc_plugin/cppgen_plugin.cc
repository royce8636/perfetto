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
#include <map>
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
static constexpr auto TYPE_SINT32 = FieldDescriptor::TYPE_SINT32;
static constexpr auto TYPE_SINT64 = FieldDescriptor::TYPE_SINT64;

static const char kHeader[] =
    "// DO NOT EDIT. Autogenerated by Perfetto cppgen_plugin\n";

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
  std::string GetProtozeroSetter(const FieldDescriptor* field) const;
  std::string GetPackedBuffer(const FieldDescriptor* field) const;
  std::string GetPackedWireType(const FieldDescriptor* field) const;

  void GenEnum(const EnumDescriptor*, Printer*) const;
  void GenEnumAliases(const EnumDescriptor*, Printer*) const;
  void GenClassDecl(const Descriptor*, Printer*) const;
  void GenClassDef(const Descriptor*, Printer*) const;

  std::vector<std::string> GetNamespaces(const FileDescriptor* file) const {
    std::string pkg = file->package() + wrapper_namespace_;
    return SplitString(pkg, ".");
  }

  template <typename T = Descriptor>
  std::string GetFullName(const T* msg, bool with_namespace = false) const {
    std::string full_type;
    full_type.append(msg->name());
    for (const Descriptor* par = msg->containing_type(); par;
         par = par->containing_type()) {
      full_type.insert(0, par->name() + "_");
    }
    if (with_namespace) {
      std::string prefix;
      for (const std::string& ns : GetNamespaces(msg->file())) {
        prefix += ns + "::";
      }
      full_type = prefix + full_type;
    }
    return full_type;
  }

  mutable std::string wrapper_namespace_;
};

CppObjGenerator::CppObjGenerator() = default;
CppObjGenerator::~CppObjGenerator() = default;

bool CppObjGenerator::Generate(const google::protobuf::FileDescriptor* file,
                               const std::string& options,
                               GeneratorContext* context,
                               std::string* error) const {
  for (const std::string& option : SplitString(options, ",")) {
    std::vector<std::string> option_pair = SplitString(option, "=");
    if (option_pair[0] == "wrapper_namespace") {
      wrapper_namespace_ =
          option_pair.size() == 2 ? "." + option_pair[1] : std::string();
    } else {
      *error = "Unknown plugin option: " + option_pair[0];
      return false;
    }
  }

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
  h_printer.Print("#include <bitset>\n");
  h_printer.Print("#include <vector>\n");
  h_printer.Print("#include <string>\n");
  h_printer.Print("#include <type_traits>\n\n");
  h_printer.Print("#include \"perfetto/protozero/cpp_message_obj.h\"\n");
  h_printer.Print("#include \"perfetto/protozero/copyable_ptr.h\"\n");
  h_printer.Print("#include \"perfetto/base/export.h\"\n\n");

  cc_printer.Print("#include \"perfetto/protozero/message.h\"\n");
  cc_printer.Print(
      "#include \"perfetto/protozero/packed_repeated_fields.h\"\n");
  cc_printer.Print("#include \"perfetto/protozero/proto_decoder.h\"\n");
  cc_printer.Print("#include \"perfetto/protozero/scattered_heap_buffer.h\"\n");
  cc_printer.Print(kHeader);
  cc_printer.Print("#if defined(__GNUC__) || defined(__clang__)\n");
  cc_printer.Print("#pragma GCC diagnostic push\n");
  cc_printer.Print("#pragma GCC diagnostic ignored \"-Wfloat-equal\"\n");
  cc_printer.Print("#endif\n");

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

  // Recursively traverse all imports and turn them into #include(s).
  std::vector<const FileDescriptor*> imports_to_visit;
  std::set<const FileDescriptor*> imports_visited;
  imports_to_visit.push_back(file);

  while (!imports_to_visit.empty()) {
    const FileDescriptor* cur = imports_to_visit.back();
    imports_to_visit.pop_back();
    imports_visited.insert(cur);
    std::string base_name = StripSuffix(cur->name(), ".proto");
    cc_printer.Print("#include \"$f$.gen.h\"\n", "f", base_name);
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

  for (int i = 0; i < file->enum_type_count(); i++)
    add_enum(file->enum_type(i));

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
  // Note: do NOT add #includes to other generated headers (either .gen.h or
  // .pbzero.h). Doing so is extremely hard to handle at the build-system level
  // and requires propagating public_deps everywhere.
  cc_printer.Print("\n");

  // -- Begin of fwd declarations.

  // Build up the map of forward declarations.
  std::multimap<std::string /*namespace*/, std::string /*decl*/> fwd_decls;
  enum FwdType { kClass, kEnum };
  auto add_fwd_decl = [&fwd_decls](FwdType cpp_type,
                                   const std::string& full_name) {
    auto dot = full_name.rfind("::");
    PERFETTO_CHECK(dot != std::string::npos);
    auto package = full_name.substr(0, dot);
    auto name = full_name.substr(dot + 2);
    if (cpp_type == kClass) {
      fwd_decls.emplace(package, "class " + name + ";");
    } else {
      PERFETTO_CHECK(cpp_type == kEnum);
      fwd_decls.emplace(package, "enum " + name + " : int;");
    }
  };

  add_fwd_decl(kClass, "protozero::Message");
  for (const Descriptor* msg : all_types) {
    add_fwd_decl(kClass, GetFullName(msg, true));
  }
  for (const EnumDescriptor* enm : all_enums) {
    add_fwd_decl(kEnum, GetFullName(enm, true));
  }

  // Emit forward declarations grouping by package.
  std::string last_package;
  auto close_last_package = [&last_package, &h_printer] {
    if (!last_package.empty()) {
      for (const std::string& ns : SplitString(last_package, "::"))
        h_printer.Print("}  // namespace $ns$\n", "ns", ns);
      h_printer.Print("\n");
    }
  };
  for (const auto& kv : fwd_decls) {
    const std::string& package = kv.first;
    if (package != last_package) {
      close_last_package();
      last_package = package;
      for (const std::string& ns : SplitString(package, "::"))
        h_printer.Print("namespace $ns$ {\n", "ns", ns);
    }
    h_printer.Print("$decl$\n", "decl", kv.second);
  }
  close_last_package();

  // -- End of fwd declarations.

  for (const std::string& ns : GetNamespaces(file)) {
    h_printer.Print("namespace $n$ {\n", "n", ns);
    cc_printer.Print("namespace $n$ {\n", "n", ns);
  }

  // Generate declarations and definitions.
  for (const EnumDescriptor* enm : local_enums)
    GenEnum(enm, &h_printer);

  for (const Descriptor* msg : local_types) {
    GenClassDecl(msg, &h_printer);
    GenClassDef(msg, &cc_printer);
  }

  for (const std::string& ns : GetNamespaces(file)) {
    h_printer.Print("}  // namespace $n$\n", "n", ns);
    cc_printer.Print("}  // namespace $n$\n", "n", ns);
  }
  cc_printer.Print("#if defined(__GNUC__) || defined(__clang__)\n");
  cc_printer.Print("#pragma GCC diagnostic pop\n");
  cc_printer.Print("#endif\n");

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

std::string CppObjGenerator::GetProtozeroSetter(
    const FieldDescriptor* field) const {
  switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
      return "AppendTinyVarInt";
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_ENUM:
      return "AppendVarInt";
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
      return "AppendSignedVarInt";
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_DOUBLE:
      return "AppendFixed";
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return "AppendString";
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      abort();
  }
  abort();
}

std::string CppObjGenerator::GetPackedBuffer(
    const FieldDescriptor* field) const {
  switch (field->type()) {
    case FieldDescriptor::TYPE_FIXED32:
      return "::protozero::PackedFixedSizeInt<uint32_t>";
    case FieldDescriptor::TYPE_SFIXED32:
      return "::protozero::PackedFixedSizeInt<int32_t>";
    case FieldDescriptor::TYPE_FIXED64:
      return "::protozero::PackedFixedSizeInt<uint64_t>";
    case FieldDescriptor::TYPE_SFIXED64:
      return "::protozero::PackedFixedSizeInt<int64_t>";
    case FieldDescriptor::TYPE_DOUBLE:
      return "::protozero::PackedFixedSizeInt<double>";
    case FieldDescriptor::TYPE_FLOAT:
      return "::protozero::PackedFixedSizeInt<float>";
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_BOOL:
      return "::protozero::PackedVarInt";
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_GROUP:
      break;  // Will abort()
  }
  abort();
}

std::string CppObjGenerator::GetPackedWireType(
    const FieldDescriptor* field) const {
  switch (field->type()) {
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FLOAT:
      return "::protozero::proto_utils::ProtoWireType::kFixed32";
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_DOUBLE:
      return "::protozero::proto_utils::ProtoWireType::kFixed64";
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_BOOL:
      return "::protozero::proto_utils::ProtoWireType::kVarInt";
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_GROUP:
      break;  // Will abort()
  }
  abort();
}

void CppObjGenerator::GenEnum(const EnumDescriptor* enum_desc,
                              Printer* p) const {
  std::string full_name = GetFullName(enum_desc);

  // When generating enums, there are two cases:
  // 1. Enums nested in a message (most frequent case), e.g.:
  //    message MyMsg { enum MyEnum { FOO=1; BAR=2; } }
  // 2. Enum defined at the package level, outside of any message.
  //
  // In the case 1, the C++ code generated by the official protobuf library is:
  // enum MyEnum {  MyMsg_MyEnum_FOO=1, MyMsg_MyEnum_BAR=2 }
  // class MyMsg { static const auto FOO = MyMsg_MyEnum_FOO; ... same for BAR }
  //
  // In the case 2, the C++ code is simply:
  // enum MyEnum { FOO=1, BAR=2 }
  // Hence this |prefix| logic.
  std::string prefix = enum_desc->containing_type() ? full_name + "_" : "";
  p->Print("enum $f$ : int {\n", "f", full_name);
  for (int e = 0; e < enum_desc->value_count(); e++) {
    const EnumValueDescriptor* value = enum_desc->value(e);
    p->Print("  $p$$n$ = $v$,\n", "p", prefix, "n", value->name(), "v",
             std::to_string(value->number()));
  }
  p->Print("};\n");
}

void CppObjGenerator::GenEnumAliases(const EnumDescriptor* enum_desc,
                                     Printer* p) const {
  int min_value = std::numeric_limits<int>::max();
  int max_value = std::numeric_limits<int>::min();
  std::string min_name;
  std::string max_name;
  std::string full_name = GetFullName(enum_desc);
  for (int e = 0; e < enum_desc->value_count(); e++) {
    const EnumValueDescriptor* value = enum_desc->value(e);
    p->Print("static constexpr auto $n$ = $f$_$n$;\n", "f", full_name, "n",
             value->name());
    if (value->number() < min_value) {
      min_value = value->number();
      min_name = full_name + "_" + value->name();
    }
    if (value->number() > max_value) {
      max_value = value->number();
      max_name = full_name + "_" + value->name();
    }
  }
  p->Print("static constexpr auto $n$_MIN = $m$;\n", "n", enum_desc->name(),
           "m", min_name);
  p->Print("static constexpr auto $n$_MAX = $m$;\n", "n", enum_desc->name(),
           "m", max_name);
}

void CppObjGenerator::GenClassDecl(const Descriptor* msg, Printer* p) const {
  std::string full_name = GetFullName(msg);
  p->Print(
      "\nclass PERFETTO_COMPONENT_EXPORT $n$ : public "
      "::protozero::CppMessageObj {\n",
      "n", full_name);
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

  // Generate constants with field numbers.
  p->Print("enum FieldNumbers {\n");
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    std::string name = field->camelcase_name();
    name[0] = perfetto::base::Uppercase(name[0]);
    p->Print("  k$n$FieldNumber = $num$,\n", "n", name, "num",
             std::to_string(field->number()));
  }
  p->Print("};\n\n");

  p->Print("$n$();\n", "n", full_name);
  p->Print("~$n$() override;\n", "n", full_name);
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
  p->Print("bool ParseFromArray(const void*, size_t) override;\n");
  p->Print("std::string SerializeAsString() const override;\n");
  p->Print("std::vector<uint8_t> SerializeAsArray() const override;\n");
  p->Print("void Serialize(::protozero::Message*) const;\n");

  // Generate accessors.
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    auto set_bit = "_has_field_.set(" + std::to_string(field->number()) + ")";
    p->Print("\n");
    if (field->options().lazy()) {
      p->Print("const std::string& $n$_raw() const { return $n$_; }\n", "n",
               field->lowercase_name());
      p->Print(
          "void set_$n$_raw(const std::string& raw) { $n$_ = raw; $s$; }\n",
          "n", field->lowercase_name(), "s", set_bit);
    } else if (!field->is_repeated()) {
      p->Print("bool has_$n$() const { return _has_field_[$bit$]; }\n", "n",
               field->lowercase_name(), "bit", std::to_string(field->number()));
      if (field->type() == TYPE_MESSAGE) {
        p->Print("$t$ $n$() const { return *$n$_; }\n", "t",
                 GetCppType(field, true), "n", field->lowercase_name());
        p->Print("$t$* mutable_$n$() { $s$; return $n$_.get(); }\n", "t",
                 GetCppType(field, false), "n", field->lowercase_name(), "s",
                 set_bit);
      } else {
        p->Print("$t$ $n$() const { return $n$_; }\n", "t",
                 GetCppType(field, true), "n", field->lowercase_name());
        p->Print("void set_$n$($t$ value) { $n$_ = value; $s$; }\n", "t",
                 GetCppType(field, true), "n", field->lowercase_name(), "s",
                 set_bit);
        if (field->type() == FieldDescriptor::TYPE_BYTES) {
          p->Print(
              "void set_$n$(const void* p, size_t s) { "
              "$n$_.assign(reinterpret_cast<const char*>(p), s); $s$; }\n",
              "n", field->lowercase_name(), "s", set_bit);
        }
      }
    } else {  // is_repeated()
      p->Print("const std::vector<$t$>& $n$() const { return $n$_; }\n", "t",
               GetCppType(field, false), "n", field->lowercase_name());
      p->Print("std::vector<$t$>* mutable_$n$() { return &$n$_; }\n", "t",
               GetCppType(field, false), "n", field->lowercase_name());

      // Generate accessors for repeated message types in the .cc file so that
      // the header doesn't depend on the full definition of all nested types.
      if (field->type() == TYPE_MESSAGE) {
        p->Print("int $n$_size() const;\n", "t", GetCppType(field, false), "n",
                 field->lowercase_name());
        p->Print("void clear_$n$();\n", "n", field->lowercase_name());
        p->Print("$t$* add_$n$();\n", "t", GetCppType(field, false), "n",
                 field->lowercase_name());
      } else {  // Primitive type.
        p->Print(
            "int $n$_size() const { return static_cast<int>($n$_.size()); }\n",
            "t", GetCppType(field, false), "n", field->lowercase_name());
        p->Print("void clear_$n$() { $n$_.clear(); }\n", "n",
                 field->lowercase_name());
        p->Print("void add_$n$($t$ value) { $n$_.emplace_back(value); }\n", "t",
                 GetCppType(field, false), "n", field->lowercase_name());
        // TODO(primiano): this should be done only for TYPE_MESSAGE.
        // Unfortuntely we didn't realize before and now we have a bunch of code
        // that does: *msg->add_int_value() = 42 instead of
        // msg->add_int_value(42).
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
  int max_field_id = 1;
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    max_field_id = std::max(max_field_id, field->number());
    if (field->options().lazy()) {
      p->Print("std::string $n$_;  // [lazy=true]\n", "n",
               field->lowercase_name());
    } else if (!field->is_repeated()) {
      std::string type = GetCppType(field, false);
      if (field->type() == TYPE_MESSAGE) {
        type = "::protozero::CopyablePtr<" + type + ">";
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

  p->Print("\nstd::bitset<$id$> _has_field_{};\n", "id",
           std::to_string(max_field_id + 1));

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

  // Accessors for repeated message fields.
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    if (field->options().lazy() || !field->is_repeated() ||
        field->type() != TYPE_MESSAGE) {
      continue;
    }
    p->Print(
        "int $c$::$n$_size() const { return static_cast<int>($n$_.size()); }\n",
        "c", full_name, "t", GetCppType(field, false), "n",
        field->lowercase_name());
    p->Print("void $c$::clear_$n$() { $n$_.clear(); }\n", "c", full_name, "n",
             field->lowercase_name());
    p->Print(
        "$t$* $c$::add_$n$() { $n$_.emplace_back(); return &$n$_.back(); }\n",
        "c", full_name, "t", GetCppType(field, false), "n",
        field->lowercase_name());
  }

  std::string proto_type = GetFullName(msg, true);

  // Generate the ParseFromArray() method definition.
  p->Print("bool $f$::ParseFromArray(const void* raw, size_t size) {\n", "f",
           full_name);
  p->Indent();
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    if (field->is_repeated())
      p->Print("$n$_.clear();\n", "n", field->lowercase_name());
  }
  p->Print("unknown_fields_.clear();\n");
  p->Print("bool packed_error = false;\n");
  p->Print("\n");
  p->Print("::protozero::ProtoDecoder dec(raw, size);\n");
  p->Print("for (auto field = dec.ReadField(); field.valid(); ");
  p->Print("field = dec.ReadField()) {\n");
  p->Indent();
  p->Print("if (field.id() < _has_field_.size()) {\n");
  p->Print("  _has_field_.set(field.id());\n");
  p->Print("}\n");
  p->Print("switch (field.id()) {\n");
  p->Indent();
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    p->Print("case $id$ /* $n$ */:\n", "id", std::to_string(field->number()),
             "n", field->lowercase_name());
    p->Indent();
    if (field->options().lazy()) {
      p->Print("$n$_ = field.as_std_string();\n", "n", field->lowercase_name());
    } else {
      std::string statement;
      if (field->type() == TYPE_MESSAGE) {
        statement = "$rval$.ParseFromArray(field.data(), field.size());\n";
      } else {
        if (field->type() == TYPE_SINT32 || field->type() == TYPE_SINT64) {
          // sint32/64 fields are special and need to be zig-zag-decoded.
          statement = "field.get_signed(&$rval$);\n";
        } else {
          statement = "field.get(&$rval$);\n";
        }
      }
      if (field->is_packed()) {
        PERFETTO_CHECK(field->is_repeated());
        if (field->type() == TYPE_SINT32 || field->type() == TYPE_SINT64) {
          PERFETTO_FATAL("packed signed (zigzag) fields are not supported");
        }
        p->Print(
            "for (::protozero::PackedRepeatedFieldIterator<$w$, $c$> "
            "rep(field.data(), field.size(), &packed_error); rep; ++rep) {\n",
            "w", GetPackedWireType(field), "c", GetCppType(field, false));
        p->Print("  $n$_.emplace_back(*rep);\n", "n", field->lowercase_name());
        p->Print("}\n");
      } else if (field->is_repeated()) {
        p->Print("$n$_.emplace_back();\n", "n", field->lowercase_name());
        p->Print(statement.c_str(), "rval",
                 field->lowercase_name() + "_.back()");
      } else if (field->type() == TYPE_MESSAGE) {
        p->Print(statement.c_str(), "rval",
                 "(*" + field->lowercase_name() + "_)");
      } else {
        p->Print(statement.c_str(), "rval", field->lowercase_name() + "_");
      }
    }
    p->Print("break;\n");
    p->Outdent();
  }  // for (field)
  p->Print("default:\n");
  p->Print("  field.SerializeAndAppendTo(&unknown_fields_);\n");
  p->Print("  break;\n");
  p->Outdent();
  p->Print("}\n");  // switch(field.id)
  p->Outdent();
  p->Print("}\n");                                           // for(field)
  p->Print("return !packed_error && !dec.bytes_left();\n");  // for(field)
  p->Outdent();
  p->Print("}\n\n");

  // Generate the SerializeAsString() method definition.
  p->Print("std::string $f$::SerializeAsString() const {\n", "f", full_name);
  p->Indent();
  p->Print("::protozero::HeapBuffered<::protozero::Message> msg;\n");
  p->Print("Serialize(msg.get());\n");
  p->Print("return msg.SerializeAsString();\n");
  p->Outdent();
  p->Print("}\n\n");

  // Generate the SerializeAsArray() method definition.
  p->Print("std::vector<uint8_t> $f$::SerializeAsArray() const {\n", "f",
           full_name);
  p->Indent();
  p->Print("::protozero::HeapBuffered<::protozero::Message> msg;\n");
  p->Print("Serialize(msg.get());\n");
  p->Print("return msg.SerializeAsArray();\n");
  p->Outdent();
  p->Print("}\n\n");

  // Generate the Serialize() method that writes the fields into the passed
  // protozero |msg| write-only interface |msg|.
  p->Print("void $f$::Serialize(::protozero::Message* msg) const {\n", "f",
           full_name);
  p->Indent();
  for (int i = 0; i < msg->field_count(); i++) {
    const FieldDescriptor* field = msg->field(i);
    std::map<std::string, std::string> args;
    args["id"] = std::to_string(field->number());
    args["n"] = field->lowercase_name();
    p->Print(args, "// Field $id$: $n$\n");
    if (field->is_packed()) {
      PERFETTO_CHECK(field->is_repeated());
      p->Print("{\n");
      p->Indent();
      p->Print("$p$ pack;\n", "p", GetPackedBuffer(field));
      p->Print(args, "for (auto& it : $n$_)\n");
      p->Print(args, "  pack.Append(it);\n");
      p->Print(args, "msg->AppendBytes($id$, pack.data(), pack.size());\n");
      p->Outdent();
      p->Print("}\n");
    } else {
      if (field->is_repeated()) {
        p->Print(args, "for (auto& it : $n$_) {\n");
        args["lvalue"] = "it";
        args["rvalue"] = "it";
      } else {
        p->Print(args, "if (_has_field_[$id$]) {\n");
        args["lvalue"] = "(*" + field->lowercase_name() + "_)";
        args["rvalue"] = field->lowercase_name() + "_";
      }
      p->Indent();
      if (field->options().lazy()) {
        p->Print(args, "msg->AppendString($id$, $rvalue$);\n");
      } else if (field->type() == TYPE_MESSAGE) {
        p->Print(args,
                 "$lvalue$.Serialize("
                 "msg->BeginNestedMessage<::protozero::Message>($id$));\n");
      } else {
        args["setter"] = GetProtozeroSetter(field);
        p->Print(args, "msg->$setter$($id$, $rvalue$);\n");
      }
      p->Outdent();
      p->Print("}\n");
    }

    p->Print("\n");
  }  // for (field)
  p->Print(
      "msg->AppendRawProtoBytes(unknown_fields_.data(), "
      "unknown_fields_.size());\n");
  p->Outdent();
  p->Print("}\n\n");
}

}  // namespace
}  // namespace protozero

int main(int argc, char** argv) {
  ::protozero::CppObjGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
