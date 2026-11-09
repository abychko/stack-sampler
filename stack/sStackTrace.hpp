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

// Code addresses of one thread, leaf first, in the form pprof expects: the
// leaf is the program counter, every caller is its return address minus one,
// so that it points into the call instruction.
struct StackTrace {
  std::vector<std::uint64_t> frames;

  // A frame-pointer chain was found on the stack.
  bool chainFound = false;

  // The return address at the stack pointer was added as the first caller:
  // the leaf function runs without a frame of its own.
  bool leafCallerAdded = false;

  // The chain runs past the end of the stack copy: a longer copy gives more frames.
  bool truncated = false;
};

}  // namespace stack_sampler
