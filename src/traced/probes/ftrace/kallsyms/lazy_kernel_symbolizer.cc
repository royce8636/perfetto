/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "src/traced/probes/ftrace/kallsyms/lazy_kernel_symbolizer.h"

#include <string>

#include <unistd.h>

#include "perfetto/base/build_config.h"
#include "perfetto/base/compiler.h"
#include "perfetto/ext/base/file_utils.h"
#include "perfetto/ext/base/scoped_file.h"
#include "perfetto/ext/base/utils.h"
#include "src/traced/probes/ftrace/kallsyms/kernel_symbol_map.h"

#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID)
#include <sys/system_properties.h>
#endif

namespace perfetto {

namespace {

const char kKallsymsPath[] = "/proc/kallsyms";
const char kPtrRestrictPath[] = "/proc/sys/kernel/kptr_restrict";
const char kLowerPtrRestrictAndroidProp[] = "security.lower_kptr_restrict";

// This class takes care of temporarily lowering kptr_restrict and putting it
// back to the original value if necessary. It solves the following problem:
// When reading /proc/kallsyms on Linux/Android, the symbol addresses can be
// masked out (i.e. they are all 00000000) through the kptr_restrict file.
// On Android kptr_restrict defaults to 2. On Linux, it depends on the
// distribution. On Android we cannot simply write() kptr_restrict ourselves.
// Doing so requires the union of:
// - filesystem ACLs: kptr_restrict is rw-r--r--// and owned by root.
// - Selinux rules: kptr_restrict is labelled as proc_security and restricted.
// - CAP_SYS_ADMIN: when writing to kptr_restrict, the kernel enforces that the
//                  caller has the SYS_ADMIN capability at write() time.
// The latter would be problematic, we don't want traced_probes to have that,
// CAP_SYS_ADMIN is too broad.
// Instead, we opt for the following model: traced_probes sets an Android
// property introduced in S (security.lower_kptr_restrict); init (which
// satisfies all the requirements above) in turn sets kptr_restrict.
// On Linux and standalone builds, instead, we don't have many options. Either:
// - The system administrator takes care of lowering kptr_restrict before
//   tracing.
// - The system administrator runs traced_probes as root / CAP_SYS_ADMIN and we
//   temporarily lower and restore kptr_restrict ourselves.
// This class deals with all these cases.
class ScopedKptrUnrestrict {
 public:
  ScopedKptrUnrestrict();   // Lowers kptr_restrict if necessary.
  ~ScopedKptrUnrestrict();  // Restores the initial kptr_restrict.

 private:
  static void WriteKptrRestrict(const std::string&);
  static bool CanReadKernelSymbolAddresses();

  static const bool kUseAndroidProperty;
  std::string initial_value_;
  bool restore_on_dtor_ = true;
};

#if PERFETTO_BUILDFLAG(PERFETTO_ANDROID_BUILD)
// This is true only on Android in-tree builds (not on standalone).
const bool ScopedKptrUnrestrict::kUseAndroidProperty = true;
#else
const bool ScopedKptrUnrestrict::kUseAndroidProperty = false;
#endif

ScopedKptrUnrestrict::ScopedKptrUnrestrict() {
  if (CanReadKernelSymbolAddresses()) {
    // Everything seems to work (e.g., we are running as root and kptr_restrict
    // is < 2). Don't touching anything.
    restore_on_dtor_ = false;
    return;
  }

  if (kUseAndroidProperty) {
#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID)
    __system_property_set(kLowerPtrRestrictAndroidProp, "1");
#endif
    // Init takes some time to react to the property change.
    // Unfortunately, we cannot read kptr_restrict because of SELinux. Instead,
    // we detect this by reading the initial line of /proc/kallsyms and checking
    // that it's non-zero. This loop waits for at most 250ms (50 * 5ms)
    for (int attempt = 1; attempt <= 50; ++attempt) {
      usleep(5000);
      if (CanReadKernelSymbolAddresses())
        return;
    }
    PERFETTO_ELOG("kallsyms addresses are still masked after setting %s",
                  kLowerPtrRestrictAndroidProp);
    return;
  }  // if (kUseAndroidProperty)

  // On Linux and Android standalone, read the kptr_restrict value and lower it
  // if needed.
  bool read_res = base::ReadFile(kPtrRestrictPath, &initial_value_);
  if (!read_res) {
    PERFETTO_PLOG("Failed to read %s", kPtrRestrictPath);
    return;
  }

  // Progressively lower kptr_restrict until we can read kallsyms.
  for (int value = atoi(initial_value_.c_str()); value > 0; --value) {
    WriteKptrRestrict(std::to_string(value));
    if (CanReadKernelSymbolAddresses())
      return;
  }
}

ScopedKptrUnrestrict::~ScopedKptrUnrestrict() {
  if (!restore_on_dtor_)
    return;
  if (kUseAndroidProperty) {
#if PERFETTO_BUILDFLAG(PERFETTO_OS_ANDROID)
    __system_property_set(kLowerPtrRestrictAndroidProp, "0");
#endif
  } else if (!initial_value_.empty()) {
    WriteKptrRestrict(initial_value_);
  }
}

void ScopedKptrUnrestrict::WriteKptrRestrict(const std::string& value) {
  // Note: kptr_restrict requires O_WRONLY. O_RDWR won't work.
  PERFETTO_DCHECK(!value.empty());
  base::ScopedFile fd = base::OpenFile(kPtrRestrictPath, O_WRONLY);
  auto wsize = write(*fd, value.c_str(), value.size());
  if (wsize <= 0)
    PERFETTO_PLOG("Failed to set %s to %s", kPtrRestrictPath, value.c_str());
}

bool ScopedKptrUnrestrict::CanReadKernelSymbolAddresses() {
  base::ScopedFile fd = base::OpenFile(kKallsymsPath, O_RDONLY);
  if (!fd) {
    PERFETTO_PLOG("open(%s) failed", kKallsymsPath);
    return false;
  }
  char buf[64]{};
  // Don't just use fscanf() as that reads the whole file (b/36473442).
  auto rsize = PERFETTO_EINTR(read(*fd, buf, sizeof(buf) - 1));
  if (rsize <= 0) {
    PERFETTO_PLOG("read(%s) failed", kKallsymsPath);
    return false;
  }
  buf[static_cast<size_t>(rsize)] = '\0';
  return strtoull(buf, nullptr, 16) != 0;
}
}  // namespace

LazyKernelSymbolizer::LazyKernelSymbolizer() = default;
LazyKernelSymbolizer::~LazyKernelSymbolizer() = default;

KernelSymbolMap* LazyKernelSymbolizer::GetOrCreateKernelSymbolMap() {
  PERFETTO_DCHECK_THREAD(thread_checker_);
  if (symbol_map_)
    return symbol_map_.get();

  symbol_map_.reset(new KernelSymbolMap());

  // If kptr_restrict is set, try temporarily lifting it (it works only if
  // traced_probes is run as a privileged user).
  ScopedKptrUnrestrict kptr_unrestrict;
  symbol_map_->Parse(kKallsymsPath);
  return symbol_map_.get();
}

void LazyKernelSymbolizer::Destroy() {
  PERFETTO_DCHECK_THREAD(thread_checker_);
  symbol_map_.reset();
  base::MaybeReleaseAllocatorMemToOS();  // For Scudo, b/170217718.
}

}  // namespace perfetto
