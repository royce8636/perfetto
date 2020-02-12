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

#ifndef INCLUDE_PERFETTO_TRACING_TRACK_EVENT_CATEGORY_REGISTRY_H_
#define INCLUDE_PERFETTO_TRACING_TRACK_EVENT_CATEGORY_REGISTRY_H_

#include "perfetto/tracing/data_source.h"

#include <atomic>
#include <utility>

namespace perfetto {

// Dynamically constructed category names should marked as such through this
// container type to make it less likely for trace points to accidentally start
// using dynamic categories. Events with dynamic categories will always be
// slightly more expensive than regular events, so use them sparingly.
class DynamicCategory final {
 public:
  explicit DynamicCategory(const std::string& name_) : name(name_) {}
  explicit DynamicCategory(const char* name_) : name(name_) {}
  DynamicCategory() {}
  ~DynamicCategory() = default;

  const std::string name;
};

namespace internal {

constexpr const char* NullCategory(const char*) {
  return nullptr;
}

perfetto::DynamicCategory NullCategory(const perfetto::DynamicCategory&);

constexpr bool StringMatchesPrefix(const char* str, const char* prefix) {
  return !*str ? !*prefix
               : !*prefix ? true
                          : *str != *prefix
                                ? false
                                : StringMatchesPrefix(str + 1, prefix + 1);
}

constexpr bool IsStringInPrefixList(const char*) {
  return false;
}

template <typename... Args>
constexpr bool IsStringInPrefixList(const char* str,
                                    const char* prefix,
                                    Args... args) {
  return StringMatchesPrefix(str, prefix) ||
         IsStringInPrefixList(str, std::forward<Args>(args)...);
}

// A compile-time representation of a track event category. See
// PERFETTO_DEFINE_CATEGORIES for registering your own categories.
struct TrackEventCategory {
  const char* const name;
};

// Holds all the registered categories for one category namespace. See
// PERFETTO_DEFINE_CATEGORIES for building the registry.
class TrackEventCategoryRegistry {
 public:
  constexpr TrackEventCategoryRegistry(size_t category_count,
                                       const TrackEventCategory* categories,
                                       std::atomic<uint8_t>* state_storage)
      : categories_(categories),
        category_count_(category_count),
        state_storage_(state_storage) {
    static_assert(
        sizeof(state_storage[0].load()) * 8 >= kMaxDataSourceInstances,
        "The category state must have enough bits for all possible data source "
        "instances");
  }

  size_t category_count() const { return category_count_; }

  // Returns a category based on its index.
  const TrackEventCategory* GetCategory(size_t index) const;

  // Turn tracing on or off for the given category in a track event data source
  // instance.
  void EnableCategoryForInstance(size_t category_index,
                                 uint32_t instance_index) const;
  void DisableCategoryForInstance(size_t category_index,
                                  uint32_t instance_index) const;

  constexpr std::atomic<uint8_t>* GetCategoryState(
      size_t category_index) const {
    return &state_storage_[category_index];
  }

  // --------------------------------------------------------------------------
  // Trace point support
  // --------------------------------------------------------------------------
  //
  // (The following methods are used by the track event trace point
  // implementation and typically don't need to be called by other code.)

  // At compile time, turn a category name into an index into the registry.
  // Returns kInvalidCategoryIndex if the category was not found, or
  // kDynamicCategoryIndex if |is_dynamic| is true or a DynamicCategory was
  // passed in.
  static constexpr size_t kInvalidCategoryIndex = static_cast<size_t>(-1);
  static constexpr size_t kDynamicCategoryIndex = static_cast<size_t>(-2);
  constexpr size_t Find(const char* name,
                        bool is_dynamic,
                        size_t index = 0) const {
    return is_dynamic ? kDynamicCategoryIndex
                      : (index == category_count_)
                            ? kInvalidCategoryIndex
                            : StringEq(categories_[index].name, name)
                                  ? index
                                  : Find(name, false, index + 1);
  }

  constexpr size_t Find(const DynamicCategory&, bool) const {
    return kDynamicCategoryIndex;
  }

  // A helper for validating that a category was registered at compile time or
  // was in the allowed list of statically defined dynamic categories.
  template <size_t CategoryIndex>
  static constexpr size_t Validate() {
    static_assert(CategoryIndex != kInvalidCategoryIndex,
                  "A track event used an unknown category. Please add it to "
                  "PERFETTO_DEFINE_CATEGORIES().");
    return CategoryIndex;
  }

  constexpr bool ValidateCategories(size_t index = 0) const {
    return (index == category_count_)
               ? true
               : IsValidCategoryName(categories_[index].name)
                     ? ValidateCategories(index + 1)
                     : false;
  }

 private:
  // TODO(skyostil): Make the compile-time routines nicer with C++14.
  static constexpr bool IsValidCategoryName(const char* name) {
    return (*name == '\"' || *name == '*')
               ? false
               : *name ? IsValidCategoryName(name + 1) : true;
  }

  static constexpr bool StringEq(const char* a, const char* b) {
    return *a != *b ? false
                    : (!*a || !*b) ? (*a == *b) : StringEq(a + 1, b + 1);
  }

  const TrackEventCategory* const categories_;
  const size_t category_count_;
  std::atomic<uint8_t>* const state_storage_;
};

}  // namespace internal
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_TRACK_EVENT_CATEGORY_REGISTRY_H_
