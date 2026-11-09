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

#include <optional>
#include <vector>

#include "sSyscallState.hpp"
#include "sThreadStatus.hpp"

namespace stack_sampler {

// Reads the threads of the target and their state from /proc, without stopping
// them.
class ThreadReader {
 public:
  explicit ThreadReader(int pid) : pid_(pid) {}

  // Thread ids from /proc/<pid>/task; empty when the process is gone.
  std::vector<int> threads() const;

  std::optional<ThreadStatus> status(int tid) const;
  SyscallState syscall(int tid) const;

 private:
  int pid_;
};

}  // namespace stack_sampler
