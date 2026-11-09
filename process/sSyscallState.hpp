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

#include <array>
#include <cstdint>
#include <string>

#include "eSyscallStatus.hpp"

namespace stack_sampler {

// The parsed content of /proc/<pid>/task/<tid>/syscall.
struct SyscallState {
  SyscallStatus status = SyscallStatus::Unknown;
  long number = -1;
  std::array<std::uint64_t, 6> arguments{};
  std::uint64_t stackPointer = 0;
  std::uint64_t programCounter = 0;

  // The text as read: two reads that differ mean the thread moved between them.
  std::string raw;
};

}  // namespace stack_sampler
