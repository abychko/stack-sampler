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

#include "cFileDescriptorReader.hpp"

#include <unistd.h>

namespace stack_sampler {

namespace {

constexpr unsigned kPassesPerRefresh = 10;

}  // namespace

const std::string &FileDescriptorReader::kind(long fd) {
  const auto found = kinds_.find(fd);
  if (found != kinds_.end()) return found->second;
  return kinds_.emplace(fd, readKind(fd)).first->second;
}

void FileDescriptorReader::newPass() {
  if (++passes_ % kPassesPerRefresh == 0) kinds_.clear();
}

std::string FileDescriptorReader::readKind(long fd) const {
  if (fd < 0) return "unknown";
  const std::string path = "/proc/" + std::to_string(pid_) + "/fd/" + std::to_string(fd);
  char target[256];
  const ssize_t size = readlink(path.c_str(), target, sizeof target - 1);
  if (size <= 0) return "unknown";
  const std::string link(target, static_cast<std::size_t>(size));
  if (link.rfind("socket:", 0) == 0) return "socket";
  if (link.rfind("pipe:", 0) == 0) return "pipe";
  if (link.rfind("anon_inode:", 0) == 0) {
    std::string name = link.substr(11);
    if (name.size() > 2 && name.front() == '[' && name.back() == ']') name = name.substr(1, name.size() - 2);
    return name;
  }
  if (link.rfind("/dev/", 0) == 0) return "device";
  if (!link.empty() && link.front() == '/') return "file";
  return "unknown";
}

}  // namespace stack_sampler
