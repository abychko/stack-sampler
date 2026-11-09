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
#include <cstring>
#include <vector>

namespace stack_sampler {

// Bytes of a thread stack copied from the target, starting at `base`.
struct StackCopy {
  std::uint64_t base = 0;
  std::vector<std::uint8_t> bytes;

  std::uint64_t end() const { return base + bytes.size(); }

  // Reads the 8-byte word at the address; false outside the copy.
  bool readWord(std::uint64_t address, std::uint64_t &value) const {
    if (address < base || address + sizeof(value) > end()) return false;
    std::memcpy(&value, bytes.data() + (address - base), sizeof(value));
    return true;
  }
};

}  // namespace stack_sampler
