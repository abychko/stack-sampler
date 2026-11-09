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

#include "cThreadReader.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>

namespace stack_sampler {

namespace {

// Reads a small /proc file in one read() call, as the kernel produces it.
std::string readSmall(const std::string &path) {
  const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) return {};
  char buffer[1024];
  const ssize_t done = read(fd, buffer, sizeof buffer - 1);
  close(fd);
  return done > 0 ? std::string(buffer, static_cast<std::size_t>(done)) : std::string();
}

}  // namespace

std::vector<int> ThreadReader::threads() const {
  std::vector<int> tids;
  const std::string path = "/proc/" + std::to_string(pid_) + "/task";
  DIR *directory = opendir(path.c_str());
  if (directory == nullptr) return tids;
  while (const dirent *entry = readdir(directory)) {
    const int tid = std::atoi(entry->d_name);
    if (tid > 0) tids.push_back(tid);
  }
  closedir(directory);
  return tids;
}

std::optional<ThreadStatus> ThreadReader::status(int tid) const {
  const std::string text = readSmall("/proc/" + std::to_string(pid_) + "/task/" + std::to_string(tid) + "/stat");
  // "<tid> (<name>) <state> ...": the name may itself contain ')'.
  const std::size_t nameStart = text.find('(');
  const std::size_t nameEnd = text.rfind(')');
  if (nameStart == std::string::npos || nameEnd == std::string::npos || nameEnd < nameStart ||
      nameEnd + 2 >= text.size())
    return std::nullopt;
  ThreadStatus status;
  status.name = text.substr(nameStart + 1, nameEnd - nameStart - 1);
  status.state = text[nameEnd + 2];
  return status;
}

SyscallState ThreadReader::syscall(int tid) const {
  SyscallState state;
  state.raw = readSmall("/proc/" + std::to_string(pid_) + "/task/" + std::to_string(tid) + "/syscall");
  if (state.raw.empty()) return state;
  if (state.raw.rfind("running", 0) == 0) {
    state.status = SyscallStatus::Running;
    return state;
  }
  // "<nr> <arg1> ... <arg6> <sp> <pc>" or "-1 <sp> <pc>"; numbers after nr are hex.
  char *at = state.raw.data();
  char *end = nullptr;
  state.number = std::strtol(at, &end, 10);
  if (end == at) return state;
  std::vector<std::uint64_t> fields;
  for (at = end; *at != '\0' && *at != '\n';) {
    const std::uint64_t value = std::strtoull(at, &end, 16);
    if (end == at) break;
    fields.push_back(value);
    at = end;
  }
  if (fields.size() < 2) return state;
  state.stackPointer = fields[fields.size() - 2];
  state.programCounter = fields[fields.size() - 1];
  if (state.number == -1) {
    state.status = SyscallStatus::NotInSyscall;
    return state;
  }
  if (fields.size() != state.arguments.size() + 2) return state;
  for (std::size_t i = 0; i < state.arguments.size(); ++i) state.arguments[i] = fields[i];
  state.status = SyscallStatus::InSyscall;
  return state;
}

}  // namespace stack_sampler
