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

#include "cMemoryMap.hpp"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <string>

namespace stack_sampler {

bool MemoryMap::load() {
  const std::string path = "/proc/" + std::to_string(pid_) + "/maps";
  std::FILE *file = std::fopen(path.c_str(), "r");
  if (file == nullptr) return false;
  mappings_.clear();
  char line[4096];
  while (std::fgets(line, sizeof line, file) != nullptr) {
    Mapping mapping;
    char permissions[8] = {};
    int pathStart = 0;
    if (std::sscanf(line, "%" SCNx64 "-%" SCNx64 " %7s %" SCNx64 " %*s %*s %n", &mapping.start, &mapping.end,
                    permissions, &mapping.offset, &pathStart) < 4)
      continue;
    mapping.executable = permissions[2] == 'x';
    std::string rest = pathStart > 0 ? std::string(line + pathStart) : std::string();
    while (!rest.empty() && (rest.back() == '\n' || rest.back() == ' ')) rest.pop_back();
    mapping.path = rest;
    mappings_.push_back(std::move(mapping));
  }
  std::fclose(file);
  return !mappings_.empty();
}

const Mapping *MemoryMap::find(std::uint64_t address) const {
  const auto after = std::upper_bound(mappings_.begin(), mappings_.end(), address,
                                      [](std::uint64_t value, const Mapping &mapping) { return value < mapping.start; });
  if (after == mappings_.begin()) return nullptr;
  const Mapping &candidate = *(after - 1);
  return address < candidate.end ? &candidate : nullptr;
}

bool MemoryMap::isCode(std::uint64_t address) const {
  const Mapping *mapping = find(address);
  return mapping != nullptr && mapping->executable;
}

}  // namespace stack_sampler
