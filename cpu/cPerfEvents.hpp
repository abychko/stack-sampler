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
#include <string>
#include <vector>

#include "cPerfRing.hpp"
#include "sPerfSample.hpp"

namespace stack_sampler {

// CPU-time sampling of every CPU at a frequency, with user stacks walked by the
// kernel along frame pointers: one perf_event per CPU, so threads that start
// later are sampled as well and no thread is ever stopped. Samples of other
// processes are dropped by the reader.
class PerfEvents {
 public:
  // Empty on success, else the reason the events cannot be opened.
  std::string open(double frequency);
  void enable();
  void disable();

  // Waits until a ring wants reading or the timeout passes.
  void wait(int timeoutMilliseconds) const;
  void drain(const std::function<void(const PerfSample &)> &handler);

  std::uint64_t lost() const;
  std::size_t cpus() const { return rings_.size(); }

 private:
  std::vector<PerfRing> rings_;
};

}  // namespace stack_sampler
