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

#include "sPerfSample.hpp"

struct perf_event_attr;

namespace stack_sampler {

// One perf_event of one CPU with its ring buffer. The kernel writes samples into
// the ring; drain() reads them without stopping anything.
class PerfRing {
 public:
  PerfRing() = default;
  PerfRing(const PerfRing &) = delete;
  PerfRing &operator=(const PerfRing &) = delete;
  PerfRing(PerfRing &&other) noexcept;
  PerfRing &operator=(PerfRing &&other) = delete;
  ~PerfRing();

  // Opens the event on the CPU for every process; empty on success, else the reason.
  std::string open(perf_event_attr &attribute, int cpu, std::size_t dataPages);

  int fd() const { return fd_; }
  bool enable() const;
  bool disable() const;

  // Calls the handler for every sample in the ring and frees the space.
  void drain(const std::function<void(const PerfSample &)> &handler);

  std::uint64_t lost() const { return lost_; }

 private:
  void parseSample(const std::uint8_t *record, std::size_t size, std::uint16_t misc,
                   const std::function<void(const PerfSample &)> &handler);

  int fd_ = -1;
  void *mapping_ = nullptr;
  std::size_t mappingSize_ = 0;
  std::size_t dataSize_ = 0;
  std::uint64_t lost_ = 0;
  std::vector<std::uint8_t> record_;
  PerfSample sample_;
};

}  // namespace stack_sampler
