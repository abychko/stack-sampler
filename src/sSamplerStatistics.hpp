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

#include <cstddef>
#include <cstdint>
#include <vector>

namespace stack_sampler {

// Counters of one sampling run, printed when it ends.
struct SamplerStatistics {
  std::uint64_t passes = 0;
  std::uint64_t latePasses = 0;  // a pass took longer than the period
  std::uint64_t threads = 0;     // thread observations over all passes
  std::uint64_t blocked = 0;     // blocked threads with a sample
  std::uint64_t running = 0;
  std::uint64_t moved = 0;    // the thread changed its call while being read
  std::uint64_t noStack = 0;  // no frame-pointer chain found
  std::uint64_t leafCallers = 0;
  std::uint64_t gone = 0;  // the thread exited or could not be read
  std::vector<double> passMilliseconds;

  std::uint64_t cpuSamples = 0;
  std::uint64_t cpuOtherProcesses = 0;  // samples of other processes, dropped
  std::uint64_t cpuNoStack = 0;
  std::uint64_t cpuLost = 0;  // samples the kernel dropped: a ring was full
  std::size_t cpus = 0;
};

}  // namespace stack_sampler
