#!/usr/bin/env python3
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse

from google.protobuf import descriptor, descriptor_pb2, message_factory, reflection
from google.protobuf.pyext import _message

CLONE_THREAD = 0x00010000
CLONE_VFORK = 0x00004000
CLONE_VM = 0x00000100

# TODO(b/174825244): These magic numbers should go away.
TYPE_SLICE_BEGIN = 1
TYPE_SLICE_END = 2

RAIL_MODE_RESPONSE = 1
RAIL_MODE_ANIMATION = 2
RAIL_MODE_IDLE = 3
RAIL_MODE_LOAD = 4

PROCESS_BROWSER = 1
PROCESS_RENDERER = 2
PROCESS_GPU = 6

CHROME_THREAD_UNSPECIFIED = 0
CHROME_THREAD_MAIN = 1
CHROME_THREAD_IO = 2
CHROME_THREAD_COMPOSITOR = 8

COUNTER_THREAD_TIME_NS = 1


def ms_to_ns(time_in_ms):
  return time_in_ms * 1000000


class Trace(object):

  def __init__(self, trace):
    self.trace = trace
    self.proc_map = {}
    self.proc_map[0] = 'idle_thread'

  def add_system_info(self, arch="", fingerprint=""):
    self.packet = self.trace.packet.add()
    self.packet.system_info.utsname.machine = arch
    self.packet.system_info.android_build_fingerprint = fingerprint

  def add_ftrace_packet(self, cpu):
    self.packet = self.trace.packet.add()
    self.packet.ftrace_events.cpu = cpu

  def add_packet(self, ts=None):
    self.packet = self.trace.packet.add()
    if ts is not None:
      self.packet.timestamp = ts
    return self.packet

  def __add_ftrace_event(self, ts, tid):
    ftrace = self.packet.ftrace_events.event.add()
    ftrace.timestamp = ts
    ftrace.pid = tid
    return ftrace

  def add_rss_stat(self, ts, tid, member, size, mm_id=None, curr=None):
    ftrace = self.__add_ftrace_event(ts, tid)
    rss_stat = ftrace.rss_stat
    rss_stat.member = member
    rss_stat.size = size
    if mm_id is not None:
      rss_stat.mm_id = mm_id
    if curr is not None:
      rss_stat.curr = curr

  def add_ion_event(self, ts, tid, heap_name, len, size=0):
    ftrace = self.__add_ftrace_event(ts, tid)
    ion = ftrace.ion_heap_grow
    ion.heap_name = heap_name
    ion.len = len
    ion.total_allocated = size

  def add_oom_score_update(self, ts, oom_score_adj, pid):
    ftrace = self.__add_ftrace_event(ts, pid)
    oom_score = ftrace.oom_score_adj_update
    oom_score.comm = self.proc_map[pid]
    oom_score.oom_score_adj = oom_score_adj
    oom_score.pid = pid

  def add_sched(self,
                ts,
                prev_pid,
                next_pid,
                prev_comm=None,
                next_comm=None,
                prev_state=None):
    ftrace = self.__add_ftrace_event(ts, 0)
    ss = ftrace.sched_switch
    ss.prev_comm = prev_comm or self.proc_map[prev_pid]
    ss.prev_pid = prev_pid
    ss.next_pid = next_pid
    ss.next_comm = next_comm or self.proc_map[next_pid]
    if prev_state:
      if prev_state == 'R':
        ss.prev_state = 0
      elif prev_state == 'S':
        ss.prev_state = 1
      elif prev_state == 'U':
        ss.prev_state = 2
      else:
        raise Exception('Invalid prev state {}'.format(prev_state))

  def add_cpufreq(self, ts, freq, cpu):
    ftrace = self.__add_ftrace_event(ts, 0)
    cpufreq = ftrace.cpu_frequency
    cpufreq.state = freq
    cpufreq.cpu_id = cpu

  def add_kernel_lmk(self, ts, tid):
    ftrace = self.__add_ftrace_event(ts, tid)
    lowmemory_kill = ftrace.lowmemory_kill
    lowmemory_kill.pid = tid

  def add_sys_enter(self, ts, tid, id):
    ftrace = self.__add_ftrace_event(ts, tid)
    sys_enter = ftrace.sys_enter
    sys_enter.id = id

  def add_sys_exit(self, ts, tid, id, ret):
    ftrace = self.__add_ftrace_event(ts, tid)
    sys_exit = ftrace.sys_exit
    sys_exit.id = id
    sys_exit.ret = ret

  def add_newtask(self, ts, tid, new_tid, new_comm, flags):
    ftrace = self.__add_ftrace_event(ts, tid)
    newtask = ftrace.task_newtask
    newtask.pid = new_tid
    newtask.comm = new_comm
    newtask.clone_flags = flags

  def add_process_free(self, ts, tid, comm, prio):
    ftrace = self.__add_ftrace_event(ts, tid)
    sched_process_free = ftrace.sched_process_free
    sched_process_free.pid = tid
    sched_process_free.comm = comm
    sched_process_free.prio = prio

  def add_rename(self, ts, tid, old_comm, new_comm, oom_score_adj):
    ftrace = self.__add_ftrace_event(ts, tid)
    task_rename = ftrace.task_rename
    task_rename.pid = tid
    task_rename.oldcomm = old_comm
    task_rename.newcomm = new_comm
    task_rename.oom_score_adj = oom_score_adj

  def add_print(self, ts, tid, buf):
    ftrace = self.__add_ftrace_event(ts, tid)
    print_event = getattr(ftrace, 'print')
    print_event.buf = buf

  def add_kmalloc(self, ts, tid, bytes_alloc, bytes_req, call_site, gfp_flags,
                  ptr):
    ftrace = self.__add_ftrace_event(ts, tid)
    kmalloc = ftrace.kmalloc
    kmalloc.bytes_alloc = bytes_alloc
    kmalloc.bytes_req = bytes_req
    kmalloc.call_site = call_site
    kmalloc.gfp_flags = gfp_flags
    kmalloc.ptr = ptr

  def add_kfree(self, ts, tid, call_site, ptr):
    ftrace = self.__add_ftrace_event(ts, tid)
    kfree = ftrace.kfree
    kfree.call_site = call_site
    kfree.ptr = ptr

  def add_atrace_counter(self, ts, pid, tid, buf, cnt):
    self.add_print(ts, tid, 'C|{}|{}|{}'.format(pid, buf, cnt))

  def add_atrace_begin(self, ts, tid, pid, buf):
    self.add_print(ts, tid, 'B|{}|{}'.format(pid, buf))

  def add_atrace_end(self, ts, tid, pid):
    self.add_print(ts, tid, 'E|{}'.format(pid))

  def add_atrace_async_begin(self, ts, tid, pid, buf):
    self.add_print(ts, tid, 'S|{}|{}|0'.format(pid, buf))

  def add_atrace_async_end(self, ts, tid, pid, buf):
    self.add_print(ts, tid, 'F|{}|{}|0'.format(pid, buf))

  def add_process(self, pid, ppid, cmdline, uid=None):
    process = self.packet.process_tree.processes.add()
    process.pid = pid
    process.ppid = ppid
    process.cmdline.append(cmdline)
    if uid is not None:
      process.uid = uid
    self.proc_map[pid] = cmdline

  def add_thread(self, tid, tgid, cmdline, name=None):
    thread = self.packet.process_tree.threads.add()
    thread.tid = tid
    thread.tgid = tgid
    if name is not None:
      thread.name = name
    self.proc_map[tid] = cmdline

  def add_battery_counters(self, ts, charge_uah, cap_prct, curr_ua,
                           curr_avg_ua):
    self.packet = self.trace.packet.add()
    self.packet.timestamp = ts
    battery_count = self.packet.battery
    battery_count.charge_counter_uah = charge_uah
    battery_count.capacity_percent = cap_prct
    battery_count.current_ua = curr_ua
    battery_count.current_avg_ua = curr_avg_ua

  def add_battery_counters_no_curr_ua(self, ts, charge_uah, cap_prct,
                                      curr_avg_ua):
    self.packet = self.trace.packet.add()
    self.packet.timestamp = ts
    battery_count = self.packet.battery
    battery_count.charge_counter_uah = charge_uah
    battery_count.capacity_percent = cap_prct
    battery_count.current_avg_ua = curr_avg_ua

  def add_power_rails_desc(self, index_val, name):
    power_rails = self.packet.power_rails
    descriptor = power_rails.rail_descriptor.add()
    descriptor.index = index_val
    descriptor.rail_name = name

  def add_power_rails_data(self, ts, index_val, value):
    power_rails = self.packet.power_rails
    energy_data = power_rails.energy_data.add()
    energy_data.index = index_val
    energy_data.timestamp_ms = ts
    energy_data.energy = value

  def add_package_list(self, ts, name, uid, version_code):
    packet = self.add_packet()
    packet.timestamp = ts
    plist = packet.packages_list
    pinfo = plist.packages.add()
    pinfo.name = name
    pinfo.uid = uid
    pinfo.version_code = version_code

  def add_profile_packet(self, ts):
    packet = self.add_packet()
    packet.timestamp = ts
    return packet.profile_packet

  def add_clock_snapshot(self, clocks, seq_id=None):
    packet = self.add_packet()
    if seq_id is not None:
      packet.trusted_packet_sequence_id = seq_id
    snap = self.packet.clock_snapshot
    for k, v in clocks.items():
      clock = snap.clocks.add()
      clock.clock_id = k
      clock.timestamp = v

  def add_gpu_counter_spec(self,
                           ts,
                           counter_id,
                           name,
                           description=None,
                           unit_numerators=[],
                           unit_denominators=[]):
    packet = self.add_packet()
    packet.timestamp = ts
    gpu_counters = packet.gpu_counter_event
    counter_desc = gpu_counters.counter_descriptor
    spec = counter_desc.specs.add()
    spec.counter_id = counter_id
    spec.name = name
    if description is not None:
      spec.description = description
    spec.numerator_units.extend(unit_numerators)
    spec.denominator_units.extend(unit_denominators)

  def add_gpu_counter(self, ts, counter_id, value, clock_id=None, seq_id=None):
    packet = self.add_packet()
    packet.timestamp = ts
    if clock_id is not None:
      packet.timestamp_clock_id = clock_id
    if seq_id is not None:
      packet.trusted_packet_sequence_id = seq_id
    gpu_counters = self.packet.gpu_counter_event
    gpu_counter = gpu_counters.counters.add()
    gpu_counter.counter_id = counter_id
    gpu_counter.int_value = value

  def add_gpu_render_stages_hw_queue_spec(self, specs=[]):
    packet = self.add_packet()
    spec = self.packet.gpu_render_stage_event.specifications
    for s in specs:
      hw_queue = spec.hw_queue.add()
      hw_queue.name = s.get('name', '')
      if 'description' in s:
        hw_queue.description = s['description']

  def add_gpu_render_stages_stage_spec(self, specs=[]):
    packet = self.add_packet()
    spec = self.packet.gpu_render_stage_event.specifications
    for s in specs:
      stage = spec.stage.add()
      stage.name = s.get('name', '')
      if 'description' in s:
        stage.description = s['description']

  def add_gpu_render_stages(self,
                            ts,
                            event_id,
                            duration,
                            hw_queue_id,
                            stage_id,
                            context,
                            render_target_handle=None,
                            render_pass_handle=None,
                            render_subpass_index_mask=None,
                            command_buffer_handle=None,
                            submission_id=None,
                            extra_data={}):
    packet = self.add_packet()
    packet.timestamp = ts
    render_stage = self.packet.gpu_render_stage_event
    render_stage.event_id = event_id
    render_stage.duration = duration
    render_stage.hw_queue_id = hw_queue_id
    render_stage.stage_id = stage_id
    render_stage.context = context
    if render_target_handle is not None:
      render_stage.render_target_handle = render_target_handle
    if render_pass_handle is not None:
      render_stage.render_pass_handle = render_pass_handle
    if render_subpass_index_mask is not None:
      for mask in render_subpass_index_mask:
        render_stage.render_subpass_index_mask.append(mask)
    if command_buffer_handle is not None:
      render_stage.command_buffer_handle = command_buffer_handle
    if submission_id is not None:
      render_stage.submission_id = submission_id
    for key, value in extra_data.items():
      data = render_stage.extra_data.add()
      data.name = key
      if value is not None:
        data.value = value

  def add_vk_debug_marker(self, ts, pid, vk_device, obj_type, obj, obj_name):
    packet = self.add_packet()
    packet.timestamp = ts
    debug_marker = (self.packet.vulkan_api_event.vk_debug_utils_object_name)
    debug_marker.pid = pid
    debug_marker.vk_device = vk_device
    debug_marker.object_type = obj_type
    debug_marker.object = obj
    debug_marker.object_name = obj_name

  def add_vk_queue_submit(self, ts, dur, pid, tid, vk_queue, vk_command_buffers,
                          submission_id):
    packet = self.add_packet()
    packet.timestamp = ts
    submit = (self.packet.vulkan_api_event.vk_queue_submit)
    submit.duration_ns = dur
    submit.pid = pid
    submit.tid = tid
    for cmd in vk_command_buffers:
      submit.vk_command_buffers.append(cmd)
    submit.submission_id = submission_id

  def add_gpu_log(self, ts, severity, tag, message):
    packet = self.add_packet()
    packet.timestamp = ts
    gpu_log = self.packet.gpu_log
    gpu_log.severity = severity
    gpu_log.tag = tag
    gpu_log.log_message = message

  def add_buffer_event_packet(self, ts, buffer_id, layer_name, frame_number,
                              event_type, duration):
    packet = self.add_packet()
    packet.timestamp = ts
    buffer_event = packet.graphics_frame_event.buffer_event
    if buffer_id >= 0:
      buffer_event.buffer_id = buffer_id
    buffer_event.layer_name = layer_name
    buffer_event.frame_number = frame_number
    if event_type >= 0:
      buffer_event.type = event_type
    buffer_event.duration_ns = duration

  def add_cpu(self, freqs):
    cpu = self.packet.cpu_info.cpus.add()
    for freq in freqs:
      cpu.frequencies.append(freq)

  def add_process_stats(self, pid, freqs):
    process = self.packet.process_stats.processes.add()
    process.pid = pid
    thread = process.threads.add()
    thread.tid = pid * 10
    for index in freqs:
      thread.cpu_freq_indices.append(index)
      thread.cpu_freq_ticks.append(freqs[index])

  def add_gpu_mem_total_ftrace_event(self, pid, ts, size):
    ftrace = self.__add_ftrace_event(ts, pid)
    gpu_mem_total_ftrace_event = ftrace.gpu_mem_total
    gpu_mem_total_ftrace_event.pid = pid
    gpu_mem_total_ftrace_event.size = size

  def add_gpu_mem_total_event(self, pid, ts, size):
    packet = self.add_packet()
    packet.timestamp = ts
    gpu_mem_total_event = packet.gpu_mem_total_event
    gpu_mem_total_event.pid = pid
    gpu_mem_total_event.size = size

  def add_sched_blocked_reason(self, ts, pid, io_wait, unblock_pid):
    ftrace = self.__add_ftrace_event(ts, unblock_pid)
    sched_blocked_reason = ftrace.sched_blocked_reason
    sched_blocked_reason.pid = pid
    sched_blocked_reason.io_wait = io_wait

  def add_track_event(self,
                      name=None,
                      ts=None,
                      track=None,
                      trusted_sequence_id=0,
                      cpu_time=None):
    packet = self.add_packet(ts=ts)
    if name is not None:
      packet.track_event.name = name
    if track is not None:
      packet.track_event.track_uuid = track
    packet.trusted_packet_sequence_id = trusted_sequence_id
    if cpu_time is not None:
      packet.track_event.extra_counter_values.append(cpu_time)
    return packet

  def add_track_descriptor(self, uuid, parent=None):
    packet = self.add_packet()
    track_descriptor = packet.track_descriptor
    if uuid is not None:
      track_descriptor.uuid = uuid
    if parent is not None:
      track_descriptor.parent_uuid = parent
    return packet

  def add_process_track_descriptor(self, process_track, pid=None):
    packet = self.add_track_descriptor(process_track)
    packet.track_descriptor.process.pid = pid
    return packet

  def add_chrome_process_track_descriptor(
      self,
      process_track,
      pid=None,
      process_type=PROCESS_RENDERER,
      host_app_package_name="org.chromium.chrome"):
    packet = self.add_process_track_descriptor(process_track, pid=pid)
    chrome_process = packet.track_descriptor.chrome_process
    chrome_process.process_type = process_type
    chrome_process.host_app_package_name = host_app_package_name
    return packet

  def add_thread_track_descriptor(self,
                                  process_track,
                                  thread_track,
                                  trusted_packet_sequence_id=None,
                                  pid=None,
                                  tid=None):
    packet = self.add_track_descriptor(thread_track, parent=process_track)
    packet.trusted_packet_sequence_id = trusted_packet_sequence_id
    packet.track_descriptor.thread.pid = pid
    packet.track_descriptor.thread.tid = tid
    return packet

  def add_chrome_thread_track_descriptor(self,
                                         process_track,
                                         thread_track,
                                         trusted_packet_sequence_id=None,
                                         pid=None,
                                         tid=None,
                                         thread_type=CHROME_THREAD_UNSPECIFIED):
    packet = self.add_thread_track_descriptor(
        process_track,
        thread_track,
        trusted_packet_sequence_id=trusted_packet_sequence_id,
        pid=pid,
        tid=tid)
    return packet

  def add_trace_packet_defaults(self,
                                trusted_packet_sequence_id=None,
                                thread_track=None,
                                counter_track=None):
    packet = self.add_track_descriptor(None)
    packet.trusted_packet_sequence_id = trusted_packet_sequence_id
    track_event_defaults = packet.trace_packet_defaults.track_event_defaults
    track_event_defaults.track_uuid = thread_track
    track_event_defaults.extra_counter_track_uuids.append(counter_track)
    return packet

  def add_counter_track_descriptor(self,
                                   trusted_packet_sequence_id=None,
                                   thread_track=None,
                                   counter_track=None):
    packet = self.add_track_descriptor(counter_track, parent=thread_track)
    packet.trusted_packet_sequence_id = trusted_packet_sequence_id
    packet.track_descriptor.counter.type = COUNTER_THREAD_TIME_NS
    return packet

  def add_chrome_thread_with_cpu_counter(self,
                                         process_track,
                                         thread_track,
                                         trusted_packet_sequence_id=None,
                                         counter_track=None,
                                         pid=None,
                                         tid=None,
                                         thread_type=None):
    self.add_chrome_thread_track_descriptor(
        process_track,
        thread_track,
        trusted_packet_sequence_id=trusted_packet_sequence_id,
        pid=pid,
        tid=tid,
        thread_type=thread_type)
    self.add_trace_packet_defaults(
        trusted_packet_sequence_id=trusted_packet_sequence_id,
        counter_track=counter_track,
        thread_track=thread_track)

    self.add_counter_track_descriptor(
        trusted_packet_sequence_id=trusted_packet_sequence_id,
        counter_track=counter_track,
        thread_track=thread_track)

  def add_track_event_slice_begin(self,
                                  name,
                                  ts,
                                  track=None,
                                  trusted_sequence_id=0,
                                  cpu_time=None):
    packet = self.add_track_event(
        name,
        ts=ts,
        track=track,
        trusted_sequence_id=trusted_sequence_id,
        cpu_time=cpu_time)
    packet.track_event.type = TYPE_SLICE_BEGIN
    return packet

  def add_track_event_slice_end(self,
                                ts,
                                track=None,
                                trusted_sequence_id=0,
                                cpu_time=None):
    packet = self.add_track_event(
        ts=ts,
        track=track,
        trusted_sequence_id=trusted_sequence_id,
        cpu_time=cpu_time)
    packet.track_event.type = TYPE_SLICE_END
    return packet

  # Returns the start slice packet.
  def add_track_event_slice(self,
                            name,
                            ts,
                            dur,
                            track=None,
                            trusted_sequence_id=0,
                            cpu_start=None,
                            cpu_delta=None):

    packet = self.add_track_event_slice_begin(
        name,
        ts,
        track=track,
        trusted_sequence_id=trusted_sequence_id,
        cpu_time=cpu_start)

    if dur >= 0:
      cpu_end = cpu_start + cpu_delta if cpu_start is not None else None
      self.add_track_event_slice_end(
          ts + dur,
          track=track,
          trusted_sequence_id=trusted_sequence_id,
          cpu_time=cpu_end)

    return packet

  def add_rail_mode_slice(self, ts, dur, track, mode):
    packet = self.add_track_event_slice(
        "Scheduler.RAILMode", ts=ts, dur=dur, track=track)
    packet.track_event.chrome_renderer_scheduler_state.rail_mode = mode


def create_trace():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      'trace_descriptor', type=str, help='location of trace descriptor')
  args = parser.parse_args()

  with open(args.trace_descriptor, 'rb') as t:
    fileContent = t.read()

  file_desc_set_pb2 = descriptor_pb2.FileDescriptorSet()
  file_desc_set_pb2.MergeFromString(fileContent)

  desc_by_path = {}
  for f_desc_pb2 in file_desc_set_pb2.file:
    f_desc_pb2_encode = f_desc_pb2.SerializeToString()
    f_desc = descriptor.FileDescriptor(
        name=f_desc_pb2.name,
        package=f_desc_pb2.package,
        serialized_pb=f_desc_pb2_encode)

    for desc in f_desc.message_types_by_name.values():
      desc_by_path[desc.full_name] = desc

  trace = message_factory.MessageFactory().GetPrototype(
      desc_by_path['perfetto.protos.Trace'])()
  return Trace(trace)
