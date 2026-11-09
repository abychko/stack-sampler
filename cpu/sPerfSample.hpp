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
#include <vector>

namespace stack_sampler {

// One CPU sample from perf_event: the thread that was on the CPU and its user
// stack, walked by the kernel along frame pointers.
struct PerfSample {
  int pid = 0;
  int tid = 0;
  std::uint64_t period = 0;  // nanoseconds of CPU time the sample stands for

  // The CPU was in the kernel: time of a system call or an interrupt, charged
  // to the user stack that entered it.
  bool kernel = false;

  // User program counter first, then raw return addresses.
  std::vector<std::uint64_t> userFrames;
};

}  // namespace stack_sampler
