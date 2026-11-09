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

#include <atomic>
#include <cstdint>

#include "cBuildIdReader.hpp"
#include "cFileDescriptorReader.hpp"
#include "cFramePointerWalker.hpp"
#include "cMemoryMap.hpp"
#include "cProcessCodeSource.hpp"
#include "cProfileBuilder.hpp"
#include "cRemoteMemory.hpp"
#include "cThreadReader.hpp"
#include "cWaitClassifier.hpp"
#include "sOptions.hpp"
#include "sSamplerStatistics.hpp"
#include "sStackCopy.hpp"

namespace stack_sampler {

// Takes the wait profile of the target: at the given frequency it passes over
// all threads, and for every blocked one reads its system call and its stack
// without stopping it. Running threads are counted with their state only.
class Sampler {
 public:
  Sampler(const Options &options, ProfileBuilder &profile);

  SamplerStatistics run(const std::atomic<bool> &stopRequested);

  // Nanoseconds of wall time one sample stands for.
  static std::int64_t wallPerSample(double frequency);

 private:
  // False when the target is gone.
  bool pass(SamplerStatistics &statistics);
  void sampleThread(int tid, SamplerStatistics &statistics);
  void addMappings();

  const Options &options_;
  ProfileBuilder &profile_;
  ThreadReader threads_;
  MemoryMap map_;
  RemoteMemory memory_;
  BuildIdReader buildIds_;
  FileDescriptorReader descriptors_;
  WaitClassifier classifier_;
  ProcessCodeSource code_;
  FramePointerWalker walker_;
  StackCopy stack_;
  std::int64_t wall_;
};

}  // namespace stack_sampler
