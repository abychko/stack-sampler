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

#include "iCodeSource.hpp"
#include "sStackCopy.hpp"
#include "sStackTrace.hpp"

namespace stack_sampler {

// Walks the frame-pointer chain of a thread that is not stopped, from its stack
// pointer and program counter only: the frame pointer register is not known.
// The chain starts at the first stack word from which saved frame pointers
// rise and every saved return address is code.
class FramePointerWalker {
 public:
  explicit FramePointerWalker(const CodeSource &code) : code_(code) {}

  StackTrace walk(const StackCopy &stack, std::uint64_t stackPointer, std::uint64_t programCounter) const;

  // The bytes before the return address end with an x86-64 call instruction.
  bool followsCall(std::uint64_t returnAddress) const;

 private:
  enum class ChainEnd { Zero, OutOfCopy, Broken };

  struct Chain {
    std::vector<std::uint64_t> returnAddresses;
    ChainEnd end = ChainEnd::Broken;
  };

  Chain follow(const StackCopy &stack, std::uint64_t frame) const;
  static bool accepted(const Chain &chain);

  const CodeSource &code_;
};

}  // namespace stack_sampler
