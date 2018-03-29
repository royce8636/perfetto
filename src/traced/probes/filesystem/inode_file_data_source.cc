/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "src/traced/probes/filesystem/inode_file_data_source.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <queue>
#include <unordered_map>

#include "perfetto/base/logging.h"
#include "perfetto/base/scoped_file.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "perfetto/tracing/core/trace_writer.h"

#include "perfetto/trace/trace_packet.pbzero.h"
#include "src/traced/probes/filesystem/file_scanner.h"

namespace perfetto {
namespace {
constexpr uint64_t kScanIntervalMs = 10000;  // 10s
constexpr uint64_t kScanDelayMs = 10000;     // 10s
constexpr uint64_t kScanBatchSize = 15000;

uint64_t OrDefault(uint64_t value, uint64_t def) {
  if (value != 0)
    return value;
  return def;
}

class StaticMapDelegate : public FileScanner::Delegate {
 public:
  StaticMapDelegate(
      std::map<BlockDeviceID, std::unordered_map<Inode, InodeMapValue>>* map)
      : map_(map) {}
  ~StaticMapDelegate() {}

 private:
  bool OnInodeFound(BlockDeviceID block_device_id,
                    Inode inode_number,
                    const std::string& path,
                    protos::pbzero::InodeFileMap_Entry_Type type) {
    std::unordered_map<Inode, InodeMapValue>& inode_map =
        (*map_)[block_device_id];
    inode_map[inode_number].SetType(type);
    inode_map[inode_number].AddPath(path);
    return true;
  }
  void OnInodeScanDone() {}
  std::map<BlockDeviceID, std::unordered_map<Inode, InodeMapValue>>* map_;
};
}

void CreateStaticDeviceToInodeMap(
    const std::string& root_directory,
    std::map<BlockDeviceID, std::unordered_map<Inode, InodeMapValue>>*
        static_file_map) {
  StaticMapDelegate delegate(static_file_map);
  FileScanner scanner({root_directory}, &delegate);
  scanner.Scan();
}

void FillInodeEntry(InodeFileMap* destination,
                    Inode inode_number,
                    const InodeMapValue& inode_map_value) {
  auto* entry = destination->add_entries();
  entry->set_inode_number(inode_number);
  entry->set_type(inode_map_value.type());
  for (const auto& path : inode_map_value.paths())
    entry->add_paths(path.c_str());
}

InodeFileDataSource::InodeFileDataSource(
    DataSourceConfig source_config,
    base::TaskRunner* task_runner,
    TracingSessionID id,
    std::map<BlockDeviceID, std::unordered_map<Inode, InodeMapValue>>*
        static_file_map,
    LRUInodeCache* cache,
    std::unique_ptr<TraceWriter> writer)
    : source_config_(std::move(source_config)),
      task_runner_(task_runner),
      session_id_(id),
      static_file_map_(static_file_map),
      cache_(cache),
      writer_(std::move(writer)),
      weak_factory_(this) {}

void InodeFileDataSource::AddInodesFromStaticMap(
    BlockDeviceID block_device_id,
    std::set<Inode>* inode_numbers) {
  // Check if block device id exists in static file map
  auto static_map_entry = static_file_map_->find(block_device_id);
  if (static_map_entry == static_file_map_->end())
    return;

  uint64_t system_found_count = 0;
  for (auto it = inode_numbers->begin(); it != inode_numbers->end();) {
    Inode inode_number = *it;
    // Check if inode number exists in static file map for given block device id
    auto inode_it = static_map_entry->second.find(inode_number);
    if (inode_it == static_map_entry->second.end()) {
      ++it;
      continue;
    }
    system_found_count++;
    it = inode_numbers->erase(it);
    FillInodeEntry(AddToCurrentTracePacket(block_device_id), inode_number,
                   inode_it->second);
  }
  PERFETTO_DLOG("%" PRIu64 " inodes found in static file map",
                system_found_count);
}

void InodeFileDataSource::AddInodesFromLRUCache(
    BlockDeviceID block_device_id,
    std::set<Inode>* inode_numbers) {
  uint64_t cache_found_count = 0;
  for (auto it = inode_numbers->begin(); it != inode_numbers->end();) {
    Inode inode_number = *it;
    auto value = cache_->Get(std::make_pair(block_device_id, inode_number));
    if (value == nullptr) {
      ++it;
      continue;
    }
    cache_found_count++;
    it = inode_numbers->erase(it);
    FillInodeEntry(AddToCurrentTracePacket(block_device_id), inode_number,
                   *value);
  }
  if (cache_found_count > 0)
    PERFETTO_DLOG("%" PRIu64 " inodes found in cache", cache_found_count);
}

void InodeFileDataSource::OnInodes(
    const std::vector<std::pair<Inode, BlockDeviceID>>& inodes) {
  if (mount_points_.empty()) {
    mount_points_ = ParseMounts();
  }
  // Group inodes from FtraceMetadata by block device
  std::map<BlockDeviceID, std::set<Inode>> inode_file_maps;
  for (const auto& inodes_pair : inodes) {
    Inode inode_number = inodes_pair.first;
    BlockDeviceID block_device_id = inodes_pair.second;
    inode_file_maps[block_device_id].emplace(inode_number);
  }
  if (inode_file_maps.size() > 1)
    PERFETTO_DLOG("Saw %zu block devices.", inode_file_maps.size());

  // Write a TracePacket with an InodeFileMap proto for each block device id
  for (auto& inode_file_map_data : inode_file_maps) {
    BlockDeviceID block_device_id = inode_file_map_data.first;
    std::set<Inode>& inode_numbers = inode_file_map_data.second;
    PERFETTO_DLOG("Saw %zu unique inode numbers.", inode_numbers.size());

    // Add entries to InodeFileMap as inodes are found and resolved to their
    // paths/type
    AddInodesFromStaticMap(block_device_id, &inode_numbers);
    AddInodesFromLRUCache(block_device_id, &inode_numbers);

    if (source_config_.inode_file_config().do_not_scan())
      inode_numbers.clear();

    if (!inode_numbers.empty()) {
      // Try to piggy back the current scan.
      auto it = missing_inodes_.find(block_device_id);
      if (it != missing_inodes_.end()) {
        it->second.insert(inode_numbers.cbegin(), inode_numbers.cend());
      }
      next_missing_inodes_[block_device_id].insert(inode_numbers.cbegin(),
                                                   inode_numbers.cend());
      if (!scan_running_) {
        scan_running_ = true;
        auto weak_this = GetWeakPtr();
        task_runner_->PostDelayedTask(
            [weak_this] {
              if (!weak_this) {
                PERFETTO_DLOG("Giving up filesystem scan.");
                return;
              }
              weak_this.get()->FindMissingInodes();
            },
            GetScanDelayMs());
      }
    }
  }
}

InodeFileMap* InodeFileDataSource::AddToCurrentTracePacket(
    BlockDeviceID block_device_id) {
  if (!has_current_trace_packet_ ||
      current_block_device_id_ != block_device_id) {
    if (has_current_trace_packet_)
      current_trace_packet_->Finalize();
    current_trace_packet_ = writer_->NewTracePacket();
    current_file_map_ = current_trace_packet_->set_inode_file_map();
    has_current_trace_packet_ = true;

    // Add block device id to InodeFileMap
    current_file_map_->set_block_device_id(block_device_id);
    // Add mount points to InodeFileMap
    auto range = mount_points_.equal_range(block_device_id);
    for (std::multimap<BlockDeviceID, std::string>::iterator it = range.first;
         it != range.second; ++it)
      current_file_map_->add_mount_points(it->second.c_str());
  }
  return current_file_map_;
}

bool InodeFileDataSource::OnInodeFound(
    BlockDeviceID block_device_id,
    Inode inode_number,
    const std::string& path,
    protos::pbzero::InodeFileMap_Entry_Type type) {
  PERFETTO_DLOG("Saw %s %lu", path.c_str(), block_device_id);
  auto it = missing_inodes_.find(block_device_id);
  if (it == missing_inodes_.end())
    return true;

  PERFETTO_DLOG("Missing %lu / %lu", missing_inodes_.size(), it->second.size());
  size_t n = it->second.erase(inode_number);
  if (n == 0)
    return true;

  if (it->second.empty())
    missing_inodes_.erase(it);

  std::pair<BlockDeviceID, Inode> key{block_device_id, inode_number};
  auto cur_val = cache_->Get(key);
  if (cur_val) {
    cur_val->AddPath(path);
    FillInodeEntry(AddToCurrentTracePacket(block_device_id), inode_number,
                   *cur_val);
  } else {
    InodeMapValue new_val(InodeMapValue(type, {path}));
    cache_->Insert(key, new_val);
    FillInodeEntry(AddToCurrentTracePacket(block_device_id), inode_number,
                   new_val);
  }
  PERFETTO_DLOG("Filled %s", path.c_str());
  return !missing_inodes_.empty();
}

void InodeFileDataSource::OnInodeScanDone() {
  // Finalize the accumulated trace packets.
  current_block_device_id_ = 0;
  current_file_map_ = nullptr;
  if (has_current_trace_packet_)
    current_trace_packet_->Finalize();
  has_current_trace_packet_ = false;
  file_scanner_.reset();
  if (next_missing_inodes_.empty()) {
    scan_running_ = false;
  } else {
    auto weak_this = GetWeakPtr();
    PERFETTO_DLOG("Starting another filesystem scan.");
    task_runner_->PostDelayedTask(
        [weak_this] {
          if (!weak_this) {
            PERFETTO_DLOG("Giving up filesystem scan.");
            return;
          }
          weak_this->FindMissingInodes();
        },
        GetScanDelayMs());
  }
}

void InodeFileDataSource::AddRootsForBlockDevice(
    BlockDeviceID block_device_id,
    std::vector<std::string>* roots) {
  auto p = mount_points_.equal_range(block_device_id);
  for (auto it = p.first; it != p.second; ++it)
    roots->emplace_back(it->second);
}

void InodeFileDataSource::FindMissingInodes() {
  missing_inodes_ = std::move(next_missing_inodes_);
  std::vector<std::string> roots;
  for (auto& p : missing_inodes_)
    AddRootsForBlockDevice(p.first, &roots);

  PERFETTO_DCHECK(file_scanner_.get() == nullptr);
  auto weak_this = GetWeakPtr();
  file_scanner_ = std::unique_ptr<FileScanner>(new FileScanner(
      std::move(roots), this, GetScanIntervalMs(), GetScanBatchSize()));

  file_scanner_->Scan(task_runner_);
}

uint64_t InodeFileDataSource::GetScanIntervalMs() {
  return OrDefault(source_config_.inode_file_config().scan_interval_ms(),
                   kScanIntervalMs);
}

uint64_t InodeFileDataSource::GetScanDelayMs() {
  return OrDefault(source_config_.inode_file_config().scan_delay_ms(),
                   kScanDelayMs);
}

uint64_t InodeFileDataSource::GetScanBatchSize() {
  return OrDefault(source_config_.inode_file_config().scan_batch_size(),
                   kScanBatchSize);
}

base::WeakPtr<InodeFileDataSource> InodeFileDataSource::GetWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

}  // namespace perfetto
