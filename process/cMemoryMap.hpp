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

#include "sMapping.hpp"

namespace stack_sampler {

// The memory map of the target, read from /proc/<pid>/maps.
class MemoryMap {
 public:
  explicit MemoryMap(int pid) : pid_(pid) {}

  // Rereads the map; false when the process is gone or not accessible.
  bool load();

  const Mapping *find(std::uint64_t address) const;
  bool isCode(std::uint64_t address) const;
  const std::vector<Mapping> &mappings() const { return mappings_; }

 private:
  static const Mapping *findIn(const std::vector<Mapping> &mappings, std::uint64_t address);

  int pid_;
  std::vector<Mapping> mappings_;  // sorted by start, as the kernel lists them
  std::vector<Mapping> code_;      // the executable ones
};

}  // namespace stack_sampler
