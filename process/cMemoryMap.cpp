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
#include <cstdio>
#include <cstring>
#include <string>

namespace stack_sampler {

namespace {

// Reads a hex number and the separator after it.
bool hexField(const char *&at, std::uint64_t &value) {
  const char *start = at;
  value = 0;
  for (;; ++at) {
    const char c = *at;
    if (c >= '0' && c <= '9')
      value = value << 4 | static_cast<std::uint64_t>(c - '0');
    else if (c >= 'a' && c <= 'f')
      value = value << 4 | static_cast<std::uint64_t>(c - 'a' + 10);
    else
      break;
  }
  if (at == start) return false;
  ++at;  // '-' or ' '
  return true;
}

void skipField(const char *&at) {
  while (*at != ' ' && *at != '\0') ++at;
  while (*at == ' ') ++at;
}

}  // namespace

// Lines are "start-end perms offset dev inode   path".
bool MemoryMap::load() {
  const std::string path = "/proc/" + std::to_string(pid_) + "/maps";
  std::FILE *file = std::fopen(path.c_str(), "r");
  if (file == nullptr) return false;
  mappings_.clear();
  code_.clear();
  char line[4096];
  while (std::fgets(line, sizeof line, file) != nullptr) {
    Mapping mapping;
    const char *at = line;
    if (!hexField(at, mapping.start) || !hexField(at, mapping.end)) continue;
    if (std::strlen(at) < 5) continue;
    mapping.executable = at[2] == 'x';
    at += 5;
    if (!hexField(at, mapping.offset)) continue;
    skipField(at);  // device
    skipField(at);  // inode
    std::size_t length = std::strlen(at);
    while (length > 0 && (at[length - 1] == '\n' || at[length - 1] == ' ')) --length;
    mapping.path.assign(at, length);
    if (mapping.executable) code_.push_back(mapping);
    mappings_.push_back(std::move(mapping));
  }
  std::fclose(file);
  return !mappings_.empty();
}

const Mapping *MemoryMap::findIn(const std::vector<Mapping> &mappings, std::uint64_t address) {
  const auto after = std::upper_bound(mappings.begin(), mappings.end(), address,
                                      [](std::uint64_t value, const Mapping &mapping) { return value < mapping.start; });
  if (after == mappings.begin()) return nullptr;
  const Mapping &candidate = *(after - 1);
  return address < candidate.end ? &candidate : nullptr;
}

const Mapping *MemoryMap::find(std::uint64_t address) const { return findIn(mappings_, address); }

bool MemoryMap::isCode(std::uint64_t address) const { return findIn(code_, address) != nullptr; }

}  // namespace stack_sampler
