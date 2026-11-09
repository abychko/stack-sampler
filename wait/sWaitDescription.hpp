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

#include <cstdint>
#include <optional>
#include <string>

namespace stack_sampler {

// What a blocked thread waits for, as far as the system call tells.
struct WaitDescription {
  // futex, poll, read, write, aio, fsync, sleep, signal, accept, child,
  // restart, other; "kernel" outside a system call.
  std::string waitClass;

  // Name of the system call, or "nr=<number>" for one without a name here.
  std::string syscall;

  // Address of the futex word: threads waiting on one lock share it.
  std::optional<std::uint64_t> futex;

  // Kind of the file descriptor the call waits on (FileDescriptorReader).
  std::optional<std::string> fdKind;
};

}  // namespace stack_sampler
