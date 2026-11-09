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

#include "cRemoteMemory.hpp"

#include <sys/uio.h>

namespace stack_sampler {

std::size_t RemoteMemory::read(std::uint64_t address, std::span<std::uint8_t> bytes) const {
  if (bytes.empty()) return 0;
  iovec local{bytes.data(), bytes.size()};
  iovec remote{reinterpret_cast<void *>(address), bytes.size()};
  const ssize_t done = process_vm_readv(pid_, &local, 1, &remote, 1, 0);
  return done > 0 ? static_cast<std::size_t>(done) : 0;
}

}  // namespace stack_sampler
