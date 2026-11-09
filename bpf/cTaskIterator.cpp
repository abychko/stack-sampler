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

#include "cTaskIterator.hpp"

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "taskIterator.skel.h"

namespace stack_sampler {

TaskIterator::~TaskIterator() {
  if (link_ != nullptr) bpf_link__destroy(link_);
  if (skeleton_ != nullptr) taskIterator_bpf__destroy(skeleton_);
}

std::string TaskIterator::open(int pid) {
  libbpf_set_print(nullptr);
  skeleton_ = taskIterator_bpf__open();
  if (skeleton_ == nullptr) return "cannot open the BPF task iterator";
  skeleton_->rodata->target_tgid = pid;
  if (taskIterator_bpf__load(skeleton_) != 0) return std::string("cannot load the BPF task iterator: ") + std::strerror(errno);
  link_ = bpf_program__attach_iter(skeleton_->progs.walk, nullptr);
  if (link_ == nullptr) return std::string("cannot attach the BPF task iterator: ") + std::strerror(errno);
  return {};
}

bool TaskIterator::pass(std::vector<ssr_task_record> &records) {
  records.clear();
  const int fd = bpf_iter_create(bpf_link__fd(link_));
  if (fd < 0) return false;
  if (buffer_.empty()) buffer_.resize(1 << 20);
  std::size_t got = 0;
  for (;;) {
    if (buffer_.size() - got < sizeof(ssr_task_record)) buffer_.resize(buffer_.size() * 2);
    const ssize_t done = read(fd, buffer_.data() + got, buffer_.size() - got);
    if (done <= 0) break;
    got += static_cast<std::size_t>(done);
  }
  close(fd);
  records.resize(got / sizeof(ssr_task_record));
  std::memcpy(records.data(), buffer_.data(), records.size() * sizeof(ssr_task_record));
  return true;
}

}  // namespace stack_sampler
