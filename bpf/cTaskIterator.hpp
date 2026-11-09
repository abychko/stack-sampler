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

#include <string>
#include <vector>

#include "sTaskRecord.h"

struct taskIterator_bpf;
struct bpf_link;

namespace stack_sampler {

// Runs the BPF task iterator over the threads of the target: one read() per
// pass returns a record per thread.
class TaskIterator {
 public:
  TaskIterator() = default;
  TaskIterator(const TaskIterator &) = delete;
  TaskIterator &operator=(const TaskIterator &) = delete;
  ~TaskIterator();

  // Empty on success, else why BPF cannot be used (no BTF, no rights, old kernel).
  std::string open(int pid);

  // Records of all threads; false when the iterator cannot be read.
  bool pass(std::vector<ssr_task_record> &records);

 private:
  taskIterator_bpf *skeleton_ = nullptr;
  bpf_link *link_ = nullptr;
  std::vector<char> buffer_;
};

}  // namespace stack_sampler
