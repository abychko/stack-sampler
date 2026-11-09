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
#include <string>

namespace stack_sampler {

// One line of /proc/<pid>/maps.
struct Mapping {
  std::uint64_t start = 0;
  std::uint64_t end = 0;
  std::uint64_t offset = 0;
  bool executable = false;
  std::string path;  // empty for anonymous memory; "[stack]", "[vdso]" for special ones
};

}  // namespace stack_sampler
