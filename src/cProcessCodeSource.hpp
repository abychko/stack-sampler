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

#include "cMemoryMap.hpp"
#include "cRemoteMemory.hpp"
#include "iCodeSource.hpp"

namespace stack_sampler {

// The code of the target process: executable mappings and their bytes.
class ProcessCodeSource : public CodeSource {
 public:
  ProcessCodeSource(const MemoryMap &map, const RemoteMemory &memory) : map_(map), memory_(memory) {}

  bool isCode(std::uint64_t address) const override { return map_.isCode(address); }
  bool readCode(std::uint64_t address, std::span<std::uint8_t> bytes) const override;

 private:
  const MemoryMap &map_;
  const RemoteMemory &memory_;
};

}  // namespace stack_sampler
