// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PROCFS_UTILS_H_
#define PROCFS_UTILS_H_

#include <map>
#include <memory>
#include <string>

#include "process_info.h"

namespace procfs_utils {

using ProcessMap = std::map<int, std::unique_ptr<ProcessInfo>>;

// ProcFS doesn't necessarly distinguish PID vs. TID, but all threads of a
// process have the same Thread Group ID which is equal to Process ID.
int ReadTgid(int pid);
std::unique_ptr<ProcessInfo> ReadProcessInfo(int pid);
void ReadProcessThreads(ProcessInfo* process);
void SerializeProcesses(ProcessMap* processes, FILE* out);

}  // namespace procfs_utils

#endif  // PROCFS_UTILS_H_
