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
#include <span>

namespace stack_sampler {

// Reads memory of the target with process_vm_readv: the target is not stopped
// and not written to.
class RemoteMemory {
 public:
  explicit RemoteMemory(int pid) : pid_(pid) {}

  // Returns the number of bytes read from the start of the range; 0 on failure.
  std::size_t read(std::uint64_t address, std::span<std::uint8_t> bytes) const;

 private:
  int pid_;
};

}  // namespace stack_sampler
