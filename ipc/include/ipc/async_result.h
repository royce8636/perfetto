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

#ifndef IPC_INCLUDE_IPC_ASYNC_RESULT_H_
#define IPC_INCLUDE_IPC_ASYNC_RESULT_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "ipc/basic_types.h"

namespace perfetto {
namespace ipc {

// Wraps the result of an asynchronous invocation. This is the equivalent of a
// std::pair<unique_ptr<T>, bool> with syntactic sugar. It is used as callback
// argument by Deferred<T>.
template <typename T = ProtoMessage>
class AsyncResult {
 public:
  static AsyncResult Create() {
    return AsyncResult(std::unique_ptr<T>(new T()));
  }

  AsyncResult(std::unique_ptr<T> msg = nullptr, bool has_more = false)
      : msg_(std::move(msg)), has_more_(has_more) {
    static_assert(std::is_base_of<ProtoMessage, T>::value, "T->ProtoMessage");
  }
  AsyncResult(AsyncResult&&) noexcept = default;
  AsyncResult& operator=(AsyncResult&&) = default;

  bool success() const { return !!msg_; }
  explicit operator bool() const { return success(); }

  bool has_more() const { return has_more_; }
  void set_has_more(bool has_more) { has_more_ = has_more; }

  void set_msg(std::unique_ptr<T> msg) { msg_ = std::move(msg); }
  T* release_msg() { return msg_.release(); }
  T* operator->() { return msg_.get(); }
  T& operator*() { return *msg_; }

 private:
  std::unique_ptr<T> msg_;
  bool has_more_ = false;
};

}  // namespace ipc
}  // namespace perfetto

#endif  // IPC_INCLUDE_IPC_ASYNC_RESULT_H_
