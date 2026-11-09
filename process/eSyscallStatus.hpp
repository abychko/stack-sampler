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

namespace stack_sampler {

// What /proc/<pid>/task/<tid>/syscall says about a thread.
enum class SyscallStatus {
  // The thread is gone or the file cannot be read.
  Unknown,

  // The thread is on a CPU: the kernel does not report its registers.
  Running,

  // Blocked inside a system call: number, arguments, SP and PC are known.
  InSyscall,

  // Blocked in the kernel outside a system call (a page fault, for example):
  // SP and PC are known.
  NotInSyscall,
};

}  // namespace stack_sampler
