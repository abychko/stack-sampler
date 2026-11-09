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
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "sSyscallState.hpp"
#include "sThreadStatus.hpp"

namespace stack_sampler {

// Reads the threads of the target and their state from /proc, without stopping
// them. The stat and syscall files of every thread stay open between passes and
// are reread from offset 0: the kernel regenerates them on every read.
class ThreadReader {
 public:
  explicit ThreadReader(int pid) : pid_(pid) {}
  ThreadReader(const ThreadReader &) = delete;
  ThreadReader &operator=(const ThreadReader &) = delete;
  ~ThreadReader();

  // Thread ids from /proc/<pid>/task; empty when the process is gone. Files of
  // threads that are gone are closed.
  std::vector<int> threads();

  std::optional<ThreadStatus> status(int tid);
  SyscallState syscall(int tid);

  // Rereads the syscall file: true when the text is still the given one.
  bool syscallUnchanged(int tid, const std::string &raw);

 private:
  enum File { kStat = 0, kSyscall = 1 };

  std::string read(int tid, File file);

  int pid_;
  std::unordered_map<int, std::array<int, 2>> files_;
  bool keepOpen_ = true;  // false once the descriptor limit is reached
};

}  // namespace stack_sampler
