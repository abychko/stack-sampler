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

#include "cWaitClassifier.hpp"

#include <poll.h>
#include <sys/syscall.h>

#include <array>
#include <cstring>
#include <string_view>

namespace stack_sampler {

namespace {

// How the arguments of a call are read.
enum class Argument { None, Descriptor, Futex, PollArray };

struct Call {
  long number;
  std::string_view name;
  std::string_view waitClass;
  Argument argument;
};

// System calls a thread blocks in, by the numbers of the build architecture.
constexpr Call kCalls[] = {
    {SYS_futex, "futex", "futex", Argument::Futex},
#ifdef SYS_futex_waitv
    {SYS_futex_waitv, "futex_waitv", "futex", Argument::None},
#endif
#ifdef SYS_poll
    {SYS_poll, "poll", "poll", Argument::PollArray},
#endif
    {SYS_ppoll, "ppoll", "poll", Argument::PollArray},
#ifdef SYS_select
    {SYS_select, "select", "poll", Argument::None},
#endif
    {SYS_pselect6, "pselect6", "poll", Argument::None},
#ifdef SYS_epoll_wait
    {SYS_epoll_wait, "epoll_wait", "poll", Argument::Descriptor},
#endif
    {SYS_epoll_pwait, "epoll_pwait", "poll", Argument::Descriptor},
#ifdef SYS_epoll_pwait2
    {SYS_epoll_pwait2, "epoll_pwait2", "poll", Argument::Descriptor},
#endif
    {SYS_read, "read", "read", Argument::Descriptor},
    {SYS_readv, "readv", "read", Argument::Descriptor},
    {SYS_pread64, "pread64", "read", Argument::Descriptor},
    {SYS_preadv, "preadv", "read", Argument::Descriptor},
    {SYS_preadv2, "preadv2", "read", Argument::Descriptor},
    {SYS_recvfrom, "recvfrom", "read", Argument::Descriptor},
    {SYS_recvmsg, "recvmsg", "read", Argument::Descriptor},
    {SYS_write, "write", "write", Argument::Descriptor},
    {SYS_writev, "writev", "write", Argument::Descriptor},
    {SYS_pwrite64, "pwrite64", "write", Argument::Descriptor},
    {SYS_pwritev, "pwritev", "write", Argument::Descriptor},
    {SYS_pwritev2, "pwritev2", "write", Argument::Descriptor},
    {SYS_sendto, "sendto", "write", Argument::Descriptor},
    {SYS_sendmsg, "sendmsg", "write", Argument::Descriptor},
    {SYS_io_getevents, "io_getevents", "aio", Argument::None},
    {SYS_io_pgetevents, "io_pgetevents", "aio", Argument::None},
    {SYS_io_uring_enter, "io_uring_enter", "aio", Argument::Descriptor},
    {SYS_fsync, "fsync", "fsync", Argument::Descriptor},
    {SYS_fdatasync, "fdatasync", "fsync", Argument::Descriptor},
    {SYS_sync_file_range, "sync_file_range", "fsync", Argument::Descriptor},
    {SYS_nanosleep, "nanosleep", "sleep", Argument::None},
    {SYS_clock_nanosleep, "clock_nanosleep", "sleep", Argument::None},
    {SYS_rt_sigtimedwait, "rt_sigtimedwait", "signal", Argument::None},
    {SYS_rt_sigsuspend, "rt_sigsuspend", "signal", Argument::None},
#ifdef SYS_pause
    {SYS_pause, "pause", "signal", Argument::None},
#endif
#ifdef SYS_accept
    {SYS_accept, "accept", "accept", Argument::Descriptor},
#endif
    {SYS_accept4, "accept4", "accept", Argument::Descriptor},
    {SYS_wait4, "wait4", "child", Argument::None},
    {SYS_waitid, "waitid", "child", Argument::None},
    {SYS_restart_syscall, "restart_syscall", "restart", Argument::None},
};

const Call *callOf(long number) {
  for (const Call &call : kCalls)
    if (call.number == number) return &call;
  return nullptr;
}

}  // namespace

WaitDescription WaitClassifier::classify(const SyscallState &state) const {
  WaitDescription description;
  if (state.status == SyscallStatus::NotInSyscall) {
    description.waitClass = "kernel";
    description.syscall = "none";
    return description;
  }
  const Call *call = callOf(state.number);
  if (call == nullptr) {
    description.waitClass = "other";
    description.syscall = "nr=" + std::to_string(state.number);
    return description;
  }
  description.waitClass = call->waitClass;
  description.syscall = call->name;
  switch (call->argument) {
    case Argument::None:
      break;
    case Argument::Futex:
      description.futex = state.arguments[0];
      break;
    case Argument::Descriptor:
      description.fdKind = descriptors_.kind(static_cast<long>(state.arguments[0]));
      break;
    case Argument::PollArray: {
      const long fd = polledDescriptor(state);
      description.fdKind = fd >= 0 ? descriptors_.kind(fd) : std::string("multiple");
      break;
    }
  }
  return description;
}

// The descriptor of a poll over exactly one descriptor (a connection waiting for
// its client), read from the pollfd array in the target; -1 otherwise.
long WaitClassifier::polledDescriptor(const SyscallState &state) const {
  if (state.arguments[1] != 1) return -1;
  std::array<std::uint8_t, sizeof(pollfd)> bytes{};
  if (memory_.read(state.arguments[0], bytes) != bytes.size()) return -1;
  pollfd entry;
  std::memcpy(&entry, bytes.data(), sizeof entry);
  return entry.fd;
}

}  // namespace stack_sampler
