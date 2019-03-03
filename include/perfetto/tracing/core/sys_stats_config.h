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

/*******************************************************************************
 * AUTOGENERATED - DO NOT EDIT
 *******************************************************************************
 * This file has been generated from the protobuf message
 * perfetto/config/sys_stats/sys_stats_config.proto
 * by
 * ../../tools/proto_to_cpp/proto_to_cpp.cc.
 * If you need to make changes here, change the .proto file and then run
 * ./tools/gen_tracing_cpp_headers_from_protos
 */

#ifndef INCLUDE_PERFETTO_TRACING_CORE_SYS_STATS_CONFIG_H_
#define INCLUDE_PERFETTO_TRACING_CORE_SYS_STATS_CONFIG_H_

#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "perfetto/base/export.h"

#include "perfetto/tracing/core/sys_stats_counters.h"

// Forward declarations for protobuf types.
namespace perfetto {
namespace protos {
class SysStatsConfig;
}
}  // namespace perfetto

namespace perfetto {

class PERFETTO_EXPORT SysStatsConfig {
 public:
  enum MeminfoCounters {
    MEMINFO_UNSPECIFIED = 0,
    MEMINFO_MEM_TOTAL = 1,
    MEMINFO_MEM_FREE = 2,
    MEMINFO_MEM_AVAILABLE = 3,
    MEMINFO_BUFFERS = 4,
    MEMINFO_CACHED = 5,
    MEMINFO_SWAP_CACHED = 6,
    MEMINFO_ACTIVE = 7,
    MEMINFO_INACTIVE = 8,
    MEMINFO_ACTIVE_ANON = 9,
    MEMINFO_INACTIVE_ANON = 10,
    MEMINFO_ACTIVE_FILE = 11,
    MEMINFO_INACTIVE_FILE = 12,
    MEMINFO_UNEVICTABLE = 13,
    MEMINFO_MLOCKED = 14,
    MEMINFO_SWAP_TOTAL = 15,
    MEMINFO_SWAP_FREE = 16,
    MEMINFO_DIRTY = 17,
    MEMINFO_WRITEBACK = 18,
    MEMINFO_ANON_PAGES = 19,
    MEMINFO_MAPPED = 20,
    MEMINFO_SHMEM = 21,
    MEMINFO_SLAB = 22,
    MEMINFO_SLAB_RECLAIMABLE = 23,
    MEMINFO_SLAB_UNRECLAIMABLE = 24,
    MEMINFO_KERNEL_STACK = 25,
    MEMINFO_PAGE_TABLES = 26,
    MEMINFO_COMMIT_LIMIT = 27,
    MEMINFO_COMMITED_AS = 28,
    MEMINFO_VMALLOC_TOTAL = 29,
    MEMINFO_VMALLOC_USED = 30,
    MEMINFO_VMALLOC_CHUNK = 31,
    MEMINFO_CMA_TOTAL = 32,
    MEMINFO_CMA_FREE = 33,
  };
  enum VmstatCounters {
    VMSTAT_UNSPECIFIED = 0,
    VMSTAT_NR_FREE_PAGES = 1,
    VMSTAT_NR_ALLOC_BATCH = 2,
    VMSTAT_NR_INACTIVE_ANON = 3,
    VMSTAT_NR_ACTIVE_ANON = 4,
    VMSTAT_NR_INACTIVE_FILE = 5,
    VMSTAT_NR_ACTIVE_FILE = 6,
    VMSTAT_NR_UNEVICTABLE = 7,
    VMSTAT_NR_MLOCK = 8,
    VMSTAT_NR_ANON_PAGES = 9,
    VMSTAT_NR_MAPPED = 10,
    VMSTAT_NR_FILE_PAGES = 11,
    VMSTAT_NR_DIRTY = 12,
    VMSTAT_NR_WRITEBACK = 13,
    VMSTAT_NR_SLAB_RECLAIMABLE = 14,
    VMSTAT_NR_SLAB_UNRECLAIMABLE = 15,
    VMSTAT_NR_PAGE_TABLE_PAGES = 16,
    VMSTAT_NR_KERNEL_STACK = 17,
    VMSTAT_NR_OVERHEAD = 18,
    VMSTAT_NR_UNSTABLE = 19,
    VMSTAT_NR_BOUNCE = 20,
    VMSTAT_NR_VMSCAN_WRITE = 21,
    VMSTAT_NR_VMSCAN_IMMEDIATE_RECLAIM = 22,
    VMSTAT_NR_WRITEBACK_TEMP = 23,
    VMSTAT_NR_ISOLATED_ANON = 24,
    VMSTAT_NR_ISOLATED_FILE = 25,
    VMSTAT_NR_SHMEM = 26,
    VMSTAT_NR_DIRTIED = 27,
    VMSTAT_NR_WRITTEN = 28,
    VMSTAT_NR_PAGES_SCANNED = 29,
    VMSTAT_WORKINGSET_REFAULT = 30,
    VMSTAT_WORKINGSET_ACTIVATE = 31,
    VMSTAT_WORKINGSET_NODERECLAIM = 32,
    VMSTAT_NR_ANON_TRANSPARENT_HUGEPAGES = 33,
    VMSTAT_NR_FREE_CMA = 34,
    VMSTAT_NR_SWAPCACHE = 35,
    VMSTAT_NR_DIRTY_THRESHOLD = 36,
    VMSTAT_NR_DIRTY_BACKGROUND_THRESHOLD = 37,
    VMSTAT_PGPGIN = 38,
    VMSTAT_PGPGOUT = 39,
    VMSTAT_PGPGOUTCLEAN = 40,
    VMSTAT_PSWPIN = 41,
    VMSTAT_PSWPOUT = 42,
    VMSTAT_PGALLOC_DMA = 43,
    VMSTAT_PGALLOC_NORMAL = 44,
    VMSTAT_PGALLOC_MOVABLE = 45,
    VMSTAT_PGFREE = 46,
    VMSTAT_PGACTIVATE = 47,
    VMSTAT_PGDEACTIVATE = 48,
    VMSTAT_PGFAULT = 49,
    VMSTAT_PGMAJFAULT = 50,
    VMSTAT_PGREFILL_DMA = 51,
    VMSTAT_PGREFILL_NORMAL = 52,
    VMSTAT_PGREFILL_MOVABLE = 53,
    VMSTAT_PGSTEAL_KSWAPD_DMA = 54,
    VMSTAT_PGSTEAL_KSWAPD_NORMAL = 55,
    VMSTAT_PGSTEAL_KSWAPD_MOVABLE = 56,
    VMSTAT_PGSTEAL_DIRECT_DMA = 57,
    VMSTAT_PGSTEAL_DIRECT_NORMAL = 58,
    VMSTAT_PGSTEAL_DIRECT_MOVABLE = 59,
    VMSTAT_PGSCAN_KSWAPD_DMA = 60,
    VMSTAT_PGSCAN_KSWAPD_NORMAL = 61,
    VMSTAT_PGSCAN_KSWAPD_MOVABLE = 62,
    VMSTAT_PGSCAN_DIRECT_DMA = 63,
    VMSTAT_PGSCAN_DIRECT_NORMAL = 64,
    VMSTAT_PGSCAN_DIRECT_MOVABLE = 65,
    VMSTAT_PGSCAN_DIRECT_THROTTLE = 66,
    VMSTAT_PGINODESTEAL = 67,
    VMSTAT_SLABS_SCANNED = 68,
    VMSTAT_KSWAPD_INODESTEAL = 69,
    VMSTAT_KSWAPD_LOW_WMARK_HIT_QUICKLY = 70,
    VMSTAT_KSWAPD_HIGH_WMARK_HIT_QUICKLY = 71,
    VMSTAT_PAGEOUTRUN = 72,
    VMSTAT_ALLOCSTALL = 73,
    VMSTAT_PGROTATED = 74,
    VMSTAT_DROP_PAGECACHE = 75,
    VMSTAT_DROP_SLAB = 76,
    VMSTAT_PGMIGRATE_SUCCESS = 77,
    VMSTAT_PGMIGRATE_FAIL = 78,
    VMSTAT_COMPACT_MIGRATE_SCANNED = 79,
    VMSTAT_COMPACT_FREE_SCANNED = 80,
    VMSTAT_COMPACT_ISOLATED = 81,
    VMSTAT_COMPACT_STALL = 82,
    VMSTAT_COMPACT_FAIL = 83,
    VMSTAT_COMPACT_SUCCESS = 84,
    VMSTAT_COMPACT_DAEMON_WAKE = 85,
    VMSTAT_UNEVICTABLE_PGS_CULLED = 86,
    VMSTAT_UNEVICTABLE_PGS_SCANNED = 87,
    VMSTAT_UNEVICTABLE_PGS_RESCUED = 88,
    VMSTAT_UNEVICTABLE_PGS_MLOCKED = 89,
    VMSTAT_UNEVICTABLE_PGS_MUNLOCKED = 90,
    VMSTAT_UNEVICTABLE_PGS_CLEARED = 91,
    VMSTAT_UNEVICTABLE_PGS_STRANDED = 92,
  };
  enum StatCounters {
    STAT_UNSPECIFIED = 0,
    STAT_CPU_TIMES = 1,
    STAT_IRQ_COUNTS = 2,
    STAT_SOFTIRQ_COUNTS = 3,
    STAT_FORK_COUNT = 4,
  };
  SysStatsConfig();
  ~SysStatsConfig();
  SysStatsConfig(SysStatsConfig&&) noexcept;
  SysStatsConfig& operator=(SysStatsConfig&&);
  SysStatsConfig(const SysStatsConfig&);
  SysStatsConfig& operator=(const SysStatsConfig&);
  bool operator==(const SysStatsConfig&) const;
  bool operator!=(const SysStatsConfig& other) const {
    return !(*this == other);
  }

  // Conversion methods from/to the corresponding protobuf types.
  void FromProto(const perfetto::protos::SysStatsConfig&);
  void ToProto(perfetto::protos::SysStatsConfig*) const;

  uint32_t meminfo_period_ms() const { return meminfo_period_ms_; }
  void set_meminfo_period_ms(uint32_t value) { meminfo_period_ms_ = value; }

  int meminfo_counters_size() const {
    return static_cast<int>(meminfo_counters_.size());
  }
  const std::vector<MeminfoCounters>& meminfo_counters() const {
    return meminfo_counters_;
  }
  std::vector<MeminfoCounters>* mutable_meminfo_counters() {
    return &meminfo_counters_;
  }
  void clear_meminfo_counters() { meminfo_counters_.clear(); }
  MeminfoCounters* add_meminfo_counters() {
    meminfo_counters_.emplace_back();
    return &meminfo_counters_.back();
  }

  uint32_t vmstat_period_ms() const { return vmstat_period_ms_; }
  void set_vmstat_period_ms(uint32_t value) { vmstat_period_ms_ = value; }

  int vmstat_counters_size() const {
    return static_cast<int>(vmstat_counters_.size());
  }
  const std::vector<VmstatCounters>& vmstat_counters() const {
    return vmstat_counters_;
  }
  std::vector<VmstatCounters>* mutable_vmstat_counters() {
    return &vmstat_counters_;
  }
  void clear_vmstat_counters() { vmstat_counters_.clear(); }
  VmstatCounters* add_vmstat_counters() {
    vmstat_counters_.emplace_back();
    return &vmstat_counters_.back();
  }

  uint32_t stat_period_ms() const { return stat_period_ms_; }
  void set_stat_period_ms(uint32_t value) { stat_period_ms_ = value; }

  int stat_counters_size() const {
    return static_cast<int>(stat_counters_.size());
  }
  const std::vector<StatCounters>& stat_counters() const {
    return stat_counters_;
  }
  std::vector<StatCounters>* mutable_stat_counters() { return &stat_counters_; }
  void clear_stat_counters() { stat_counters_.clear(); }
  StatCounters* add_stat_counters() {
    stat_counters_.emplace_back();
    return &stat_counters_.back();
  }

 private:
  uint32_t meminfo_period_ms_ = {};
  std::vector<MeminfoCounters> meminfo_counters_;
  uint32_t vmstat_period_ms_ = {};
  std::vector<VmstatCounters> vmstat_counters_;
  uint32_t stat_period_ms_ = {};
  std::vector<StatCounters> stat_counters_;

  // Allows to preserve unknown protobuf fields for compatibility
  // with future versions of .proto files.
  std::string unknown_fields_;
};

}  // namespace perfetto

#endif  // INCLUDE_PERFETTO_TRACING_CORE_SYS_STATS_CONFIG_H_
