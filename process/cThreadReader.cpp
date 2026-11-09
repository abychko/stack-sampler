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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_set>

namespace stack_sampler {

namespace {

constexpr std::size_t kReadSize = 1024;

const char *const kFileNames[] = {"stat", "syscall"};

}  // namespace

ThreadReader::~ThreadReader() {
  for (const auto &[tid, fds] : files_)
    for (int fd : fds)
      if (fd >= 0) close(fd);
}

std::vector<int> ThreadReader::threads() {
  std::vector<int> tids;
  const std::string path = "/proc/" + std::to_string(pid_) + "/task";
  DIR *directory = opendir(path.c_str());
  if (directory == nullptr) return tids;
  while (const dirent *entry = readdir(directory)) {
    const int tid = std::atoi(entry->d_name);
    if (tid > 0) tids.push_back(tid);
  }
  closedir(directory);

  const std::unordered_set<int> present(tids.begin(), tids.end());
  for (auto it = files_.begin(); it != files_.end();) {
    if (present.count(it->first) != 0) {
      ++it;
      continue;
    }
    for (int fd : it->second)
      if (fd >= 0) close(fd);
    it = files_.erase(it);
  }
  return tids;
}

// The current text of a /proc file of the thread, read in one call from offset 0.
std::string ThreadReader::read(int tid, File file) {
  char buffer[kReadSize];
  if (!keepOpen_) {
    const std::string path = "/proc/" + std::to_string(pid_) + "/task/" + std::to_string(tid) + "/" + kFileNames[file];
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return {};
    const ssize_t done = pread(fd, buffer, sizeof buffer - 1, 0);
    close(fd);
    return done > 0 ? std::string(buffer, static_cast<std::size_t>(done)) : std::string();
  }
  auto &fds = files_.try_emplace(tid, std::array<int, 2>{-1, -1}).first->second;
  if (fds[file] < 0) {
    const std::string path = "/proc/" + std::to_string(pid_) + "/task/" + std::to_string(tid) + "/" + kFileNames[file];
    fds[file] = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fds[file] < 0) {
      if (errno == EMFILE || errno == ENFILE) {
        keepOpen_ = false;  // out of descriptors: read the rest by path
        return read(tid, file);
      }
      return {};
    }
  }
  const ssize_t done = pread(fds[file], buffer, sizeof buffer - 1, 0);
  if (done > 0) return std::string(buffer, static_cast<std::size_t>(done));
  // The thread is gone: its files no longer read.
  for (int &fd : fds)
    if (fd >= 0) close(fd), fd = -1;
  return {};
}

std::optional<ThreadStatus> ThreadReader::status(int tid) {
  const std::string text = read(tid, kStat);
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

SyscallState ThreadReader::syscall(int tid) {
  SyscallState state;
  state.raw = read(tid, kSyscall);
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

bool ThreadReader::syscallUnchanged(int tid, const std::string &raw) { return read(tid, kSyscall) == raw; }

}  // namespace stack_sampler
