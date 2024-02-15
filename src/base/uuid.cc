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

#include "perfetto/ext/base/uuid.h"

#include <mutex>
#include <random>

#include "perfetto/base/time.h"
#include "perfetto/ext/base/no_destructor.h"

namespace perfetto {
namespace base {
namespace {

constexpr char kHexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
}  // namespace

// A globally unique 128-bit number.
// In the early days of perfetto we were (sorta) respecting rfc4122. Later we
// started replacing the LSB of the UUID with the statsd subscription ID in
// other parts of the codebase (see perfetto_cmd.cc) for the convenience of
// trace lookups, so rfc4122 made no sense as it just reduced entropy.
Uuid Uuidv4() {
  // Mix different sources of entropy to reduce the chances of collisions.
  // Only using boot time is not enough. Under the assumption that most traces
  // are started around the same time at boot, within a 1s window, the birthday
  // paradox gives a chance of 90% collisions with 70k traces over a 1e9 space
  // (Number of ns in a 1s window).
  // &kHexmap >> 14 is used to feed use indirectly ASLR as a source of entropy.
  // We deliberately don't use /dev/urandom as that might block for
  // unpredictable time if the system is idle.
  // The UUID does NOT need to be cryptographically secure, but random enough
  // to avoid collisions across a large number of devices.
  static std::minstd_rand rng(
      static_cast<uint32_t>(static_cast<uint64_t>(GetBootTimeNs().count()) ^
                            static_cast<uint64_t>(GetWallTimeNs().count()) ^
                            (reinterpret_cast<uintptr_t>(&kHexmap) >> 14)));
  Uuid uuid;
  auto& data = *uuid.data();

  // std::random is not thread safe and users of this class might mistakenly
  // assume Uuidv4() is thread_safe because from the outside looks like a
  // local object.
  static base::NoDestructor<std::mutex> rand_mutex;
  std::unique_lock<std::mutex> rand_lock(rand_mutex.ref());

  for (size_t i = 0; i < sizeof(data);) {
    // Note: the 32-th bit of rng() is always 0 as minstd_rand operates modulo
    // 2**31. Fill in blocks of 16b rather than 32b to not lose 1b of entropy.
    const auto rnd_data = static_cast<uint16_t>(rng());
    memcpy(&data[i], &rnd_data, sizeof(rnd_data));
    i += sizeof(rnd_data);
  }

  return uuid;
}

Uuid::Uuid() {}

Uuid::Uuid(const std::string& s) {
  PERFETTO_CHECK(s.size() == data_.size());
  memcpy(data_.data(), s.data(), s.size());
}

Uuid::Uuid(int64_t lsb, int64_t msb) {
  set_lsb_msb(lsb, msb);
}

std::string Uuid::ToString() const {
  return std::string(reinterpret_cast<const char*>(data_.data()), data_.size());
}

std::string Uuid::ToPrettyString() const {
  std::string s(data_.size() * 2 + 4, '-');
  // Format is 123e4567-e89b-12d3-a456-426655443322.
  size_t j = 0;
  for (size_t i = 0; i < data_.size(); ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10)
      j++;
    s[2 * i + j] = kHexmap[(data_[data_.size() - i - 1] & 0xf0) >> 4];
    s[2 * i + 1 + j] = kHexmap[(data_[data_.size() - i - 1] & 0x0f)];
  }
  return s;
}

}  // namespace base
}  // namespace perfetto
