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

namespace stack_sampler {

// Tells what a file descriptor of the target refers to, from the link in
// /proc/<pid>/fd: "socket", "pipe", "eventpoll" and other anonymous inodes,
// "device", "file", or "unknown".
class FileDescriptorReader {
 public:
  explicit FileDescriptorReader(int pid) : pid_(pid) {}

  std::string kind(long fd) const;

 private:
  int pid_;
};

}  // namespace stack_sampler
