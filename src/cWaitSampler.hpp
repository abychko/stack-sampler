/* Copyright (c) 2026, Alexey Bychko.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#pragma once

#include <cstdint>
#include <functional>

#include "cFileDescriptorReader.hpp"
#include "cFramePointerWalker.hpp"
#include "cMemoryMap.hpp"
#include "cProcessCodeSource.hpp"
#include "cProfileBuilder.hpp"
#include "cRemoteMemory.hpp"
#include "cThreadReader.hpp"
#include "cWaitClassifier.hpp"
#ifdef STACK_SAMPLER_HAS_BPF
#include <optional>
#include <vector>

#include "cTaskIterator.hpp"
#endif
#include "sSamplerStatistics.hpp"
#include "sStackCopy.hpp"

namespace stack_sampler {

// The wait profile: one pass over all threads of the target reads, for every
// blocked one, its system call and its stack without stopping it. Running
// threads are counted with their state only; their stacks come from the CPU
// profile.
class WaitSampler {
 public:
  // The memory map is loaded by the caller; reloadMap rereads it when the stack
  // of a thread is not in it, at most once a pass.
  WaitSampler(int pid, const MemoryMap &map, ProfileBuilder &profile, std::int64_t wallPerSample,
              std::function<bool()> reloadMap);

  // False when the target has no threads left: it is gone.
  bool pass(SamplerStatistics &statistics);

  // Nanoseconds of wall time one sample stands for.
  static std::int64_t wallPerSample(double frequency);

 private:
  void sampleThread(int tid, SamplerStatistics &statistics);
#ifdef STACK_SAMPLER_HAS_BPF
  void sampleRecord(const ssr_task_record &record, SamplerStatistics &statistics);
#endif
  void addBlocked(SampleLabels &labels, const SyscallState &state, std::vector<std::uint64_t> frames);
  const Mapping *stackMappingOf(std::uint64_t stackPointer);
  StackTrace walkStack(const Mapping &stackMapping, const SyscallState &state, SamplerStatistics &statistics);

  const MemoryMap &map_;
  ProfileBuilder &profile_;
  std::function<bool()> reloadMap_;
  bool mapReloaded_ = false;
  ThreadReader threads_;
  RemoteMemory memory_;
  FileDescriptorReader descriptors_;
  WaitClassifier classifier_;
  ProcessCodeSource code_;
  FramePointerWalker walker_;
  StackCopy stack_;
  std::int64_t wall_;
#ifdef STACK_SAMPLER_HAS_BPF
  std::optional<TaskIterator> tasks_;
  std::vector<ssr_task_record> records_;
#endif
};

}  // namespace stack_sampler
