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

#include <string>
#include <unordered_map>
#include <vector>

#include "cPerfEvents.hpp"
#include "cProfileBuilder.hpp"
#include "cThreadReader.hpp"
#include "sSamplerStatistics.hpp"

namespace stack_sampler {

// The CPU profile: CPU-time samples of the target's threads with their user
// stacks, from perf_event on every CPU. Threads are not stopped.
class CpuSampler {
 public:
  CpuSampler(int pid, ProfileBuilder &profile, double frequency) : pid_(pid), threads_(pid), profile_(profile), frequency_(frequency) {}

  // Empty on success, else why sampling cannot start.
  std::string start();
  void wait(int timeoutMilliseconds) const { events_.wait(timeoutMilliseconds); }
  void drain(SamplerStatistics &statistics);
  void stop(SamplerStatistics &statistics);

 private:
  const std::string &threadName(int tid);

  int pid_;
  ThreadReader threads_;
  ProfileBuilder &profile_;
  double frequency_;
  PerfEvents events_;
  std::unordered_map<int, std::string> names_;
  std::vector<std::uint64_t> frames_;
};

}  // namespace stack_sampler
