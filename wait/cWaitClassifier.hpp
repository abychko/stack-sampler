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

#include "cFileDescriptorReader.hpp"
#include "cRemoteMemory.hpp"
#include "sSyscallState.hpp"
#include "sWaitDescription.hpp"

namespace stack_sampler {

// Classifies the wait of a blocked thread by its system call and arguments.
class WaitClassifier {
 public:
  WaitClassifier(const RemoteMemory &memory, const FileDescriptorReader &descriptors)
      : memory_(memory), descriptors_(descriptors) {}

  WaitDescription classify(const SyscallState &state) const;

 private:
  long polledDescriptor(const SyscallState &state) const;

  const RemoteMemory &memory_;
  const FileDescriptorReader &descriptors_;
};

}  // namespace stack_sampler
