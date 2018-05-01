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

#ifndef INCLUDE_PERFETTO_BASE_SCOPED_FILE_H_
#define INCLUDE_PERFETTO_BASE_SCOPED_FILE_H_

#include "perfetto/base/build_config.h"

#include <fcntl.h>
#include <stdio.h>

#if PERFETTO_BUILDFLAG(PERFETTO_OS_WIN)
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#include <string>

#include "perfetto/base/logging.h"

namespace perfetto {
namespace base {

// RAII classes for auto-releasing fds and dirs.
template <typename T, int (*CloseFunction)(T), T InvalidValue>
class ScopedResource {
 public:
  explicit ScopedResource(T t = InvalidValue) : t_(t) {}
  ScopedResource(ScopedResource&& other) noexcept {
    t_ = other.t_;
    other.t_ = InvalidValue;
  }
  ScopedResource& operator=(ScopedResource&& other) {
    reset(other.t_);
    other.t_ = InvalidValue;
    return *this;
  }
  T get() const { return t_; }
  T operator*() const { return t_; }
  explicit operator bool() const { return t_ != InvalidValue; }
  void reset(T r = InvalidValue) {
    if (t_ != InvalidValue) {
      int res = CloseFunction(t_);
      PERFETTO_CHECK(res == 0);
    }
    t_ = r;
  }
  T release() {
    T t = t_;
    t_ = InvalidValue;
    return t;
  }
  ~ScopedResource() { reset(InvalidValue); }

 private:
  ScopedResource(const ScopedResource&) = delete;
  ScopedResource& operator=(const ScopedResource&) = delete;

  T t_;
};

#if PERFETTO_BUILDFLAG(PERFETTO_OS_WIN)
using ScopedFile = ScopedResource<int, _close, -1>;
inline static ScopedFile OpenFile(const std::string& path, int flags) {
  ScopedFile fd(open(path.c_str(), flags));
  return fd;
}
#else
using ScopedFile = ScopedResource<int, close, -1>;
// Always open a ScopedFile with O-CLOEXEC so we can safely fork and exec.
inline static ScopedFile OpenFile(const std::string& path, int flags) {
  ScopedFile fd(open(path.c_str(), flags | O_CLOEXEC));
  return fd;
}

using ScopedDir = ScopedResource<DIR*, closedir, nullptr>;
#endif

using ScopedFstream = ScopedResource<FILE*, fclose, nullptr>;

}  // namespace base
}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_BASE_SCOPED_FILE_H_
