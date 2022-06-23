/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef SRC_TRACE_PROCESSOR_IMPORTERS_COMMON_ARGS_TRANSLATION_TABLE_H_
#define SRC_TRACE_PROCESSOR_IMPORTERS_COMMON_ARGS_TRANSLATION_TABLE_H_

#include <cstdint>

#include "perfetto/ext/base/flat_hash_map.h"
#include "perfetto/ext/base/optional.h"
#include "perfetto/ext/base/string_view.h"
#include "src/trace_processor/importers/common/args_tracker.h"
#include "src/trace_processor/storage/trace_storage.h"
#include "src/trace_processor/util/proto_to_args_parser.h"

template <>
struct std::hash<std::pair<perfetto::trace_processor::MappingId, uint64_t>> {
  size_t operator()(const std::pair<perfetto::trace_processor::MappingId,
                                    uint64_t>& p) const {
    return std::hash<perfetto::trace_processor::MappingId>{}(p.first) ^
           std::hash<uint64_t>{}(p.second);
  }
};

namespace perfetto {
namespace trace_processor {

// Tracks and stores args translation rules. It allows Trace Processor
// to map for example hashes to their names.
class ArgsTranslationTable {
 public:
  using Key = util::ProtoToArgsParser::Key;
  using NativeSymbolKey = std::pair<MappingId, uint64_t>;
  struct SourceLocation {
    std::string file_name;
    std::string function_name;
    uint32_t line_number;
  };

  explicit ArgsTranslationTable(TraceStorage* storage);

  // Returns true if an arg with the given key and type requires translation.
  bool NeedsTranslation(StringId key_id, Variadic::Type type) const;

  void TranslateArgs(const ArgsTracker::CompactArgSet& arg_set,
                     ArgsTracker::BoundInserter& inserter) const;

  void AddChromeHistogramTranslationRule(uint64_t hash, base::StringView name) {
    chrome_histogram_hash_to_name_.Insert(hash, name.ToStdString());
  }
  void AddChromeUserEventTranslationRule(uint64_t hash,
                                         base::StringView action) {
    chrome_user_event_hash_to_action_.Insert(hash, action.ToStdString());
  }
  void AddChromePerformanceMarkSiteTranslationRule(uint64_t hash,
                                                   base::StringView name) {
    chrome_performance_mark_site_hash_to_name_.Insert(hash, name.ToStdString());
  }
  void AddChromePerformanceMarkMarkTranslationRule(uint64_t hash,
                                                   base::StringView name) {
    chrome_performance_mark_mark_hash_to_name_.Insert(hash, name.ToStdString());
  }
  void AddNativeSymbolTranslationRule(MappingId mapping_id,
                                      uint64_t rel_pc,
                                      const SourceLocation& loc) {
    native_symbol_to_location_.Insert(std::make_pair(mapping_id, rel_pc), loc);
  }

  base::Optional<base::StringView> TranslateChromeHistogramHashForTesting(
      uint64_t hash) const {
    return TranslateChromeHistogramHash(hash);
  }
  base::Optional<base::StringView> TranslateChromeUserEventHashForTesting(
      uint64_t hash) const {
    return TranslateChromeUserEventHash(hash);
  }
  base::Optional<base::StringView>
  TranslateChromePerformanceMarkSiteHashForTesting(uint64_t hash) const {
    return TranslateChromePerformanceMarkSiteHash(hash);
  }
  base::Optional<base::StringView>
  TranslateChromePerformanceMarkMarkHashForTesting(uint64_t hash) const {
    return TranslateChromePerformanceMarkMarkHash(hash);
  }

 private:
  enum class KeyType {
    kChromeHistogramHash = 0,
    kChromeUserEventHash = 1,
    kChromePerformanceMarkMarkHash = 2,
    kChromePerformanceMarkSiteHash = 3,
    kMojoMethodMappingId = 4,
    kMojoMethodRelPc = 5,
  };

  static constexpr char kChromeHistogramHashKey[] =
      "chrome_histogram_sample.name_hash";
  static constexpr char kChromeHistogramNameKey[] =
      "chrome_histogram_sample.name";

  static constexpr char kChromeUserEventHashKey[] =
      "chrome_user_event.action_hash";
  static constexpr char kChromeUserEventActionKey[] =
      "chrome_user_event.action";

  static constexpr char kChromePerformanceMarkSiteHashKey[] =
      "chrome_hashed_performance_mark.site_hash";
  static constexpr char kChromePerformanceMarkSiteKey[] =
      "chrome_hashed_performance_mark.site";

  static constexpr char kChromePerformanceMarkMarkHashKey[] =
      "chrome_hashed_performance_mark.mark_hash";
  static constexpr char kChromePerformanceMarkMarkKey[] =
      "chrome_hashed_performance_mark.mark";

  static constexpr char kMojoMethodMappingIdKey[] =
      "chrome_mojo_event_info.mojo_interface_method.native_symbol.mapping_id";
  static constexpr char kMojoMethodRelPcKey[] =
      "chrome_mojo_event_info.mojo_interface_method.native_symbol.rel_pc";
  static constexpr char kMojoMethodNameKey[] =
      "chrome_mojo_event_info.mojo_method_name";

  TraceStorage* storage_;
  StringId interned_chrome_histogram_hash_key_;
  StringId interned_chrome_histogram_name_key_;
  StringId interned_chrome_user_event_hash_key_;
  StringId interned_chrome_user_event_action_key_;
  StringId interned_chrome_performance_mark_site_hash_key_;
  StringId interned_chrome_performance_mark_site_key_;
  StringId interned_chrome_performance_mark_mark_hash_key_;
  StringId interned_chrome_performance_mark_mark_key_;

  StringId interned_mojo_method_mapping_id_;
  StringId interned_mojo_method_rel_pc_;
  StringId interned_mojo_method_name_;

  base::FlatHashMap<uint64_t, std::string> chrome_histogram_hash_to_name_;
  base::FlatHashMap<uint64_t, std::string> chrome_user_event_hash_to_action_;
  base::FlatHashMap<uint64_t, std::string>
      chrome_performance_mark_site_hash_to_name_;
  base::FlatHashMap<uint64_t, std::string>
      chrome_performance_mark_mark_hash_to_name_;
  base::FlatHashMap<NativeSymbolKey, SourceLocation> native_symbol_to_location_;

  // Returns the corresponding SupportedKey enum if the table knows how to
  // translate the argument with the given key and type, and nullopt otherwise.
  base::Optional<KeyType> KeyIdAndTypeToEnum(StringId key_id,
                                             Variadic::Type type) const;

  base::Optional<base::StringView> TranslateChromeHistogramHash(
      uint64_t hash) const;
  base::Optional<base::StringView> TranslateChromeUserEventHash(
      uint64_t hash) const;
  base::Optional<base::StringView> TranslateChromePerformanceMarkSiteHash(
      uint64_t hash) const;
  base::Optional<base::StringView> TranslateChromePerformanceMarkMarkHash(
      uint64_t hash) const;
  base::Optional<SourceLocation> TranslateNativeSymbol(MappingId mapping_id,
                                                       uint64_t rel_pc) const;

  void EmitMojoMethodLocation(base::Optional<uint64_t> mapping_id,
                              base::Optional<uint64_t> rel_pc,
                              ArgsTracker::BoundInserter& inserter) const;
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_IMPORTERS_COMMON_ARGS_TRANSLATION_TABLE_H_
