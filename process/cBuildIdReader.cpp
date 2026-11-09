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

#include "cBuildIdReader.hpp"

#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <vector>

namespace stack_sampler {

namespace {

constexpr std::size_t kMaximumNoteSegment = 1 << 16;

std::uint64_t alignedFour(std::uint64_t size) { return (size + 3) & ~std::uint64_t{3}; }

bool readAt(int fd, void *buffer, std::size_t size, off_t offset) {
  return pread(fd, buffer, size, offset) == static_cast<ssize_t>(size);
}

std::string buildIdOfNotes(const std::vector<std::uint8_t> &notes) {
  std::size_t at = 0;
  while (at + sizeof(Elf64_Nhdr) <= notes.size()) {
    Elf64_Nhdr header;
    std::memcpy(&header, notes.data() + at, sizeof header);
    const std::size_t name = at + sizeof header;
    const std::size_t description = name + alignedFour(header.n_namesz);
    const std::size_t next = description + alignedFour(header.n_descsz);
    if (next > notes.size()) break;
    if (header.n_type == NT_GNU_BUILD_ID && header.n_namesz == 4 &&
        std::memcmp(notes.data() + name, "GNU", 4) == 0) {
      std::string hex;
      char digits[3];
      for (std::size_t i = 0; i < header.n_descsz; ++i) {
        std::snprintf(digits, sizeof digits, "%02x", notes[description + i]);
        hex += digits;
      }
      return hex;
    }
    at = next;
  }
  return {};
}

}  // namespace

const std::string &BuildIdReader::read(const Mapping &mapping) {
  const auto found = cache_.find(mapping.path);
  if (found != cache_.end()) return found->second;

  // The target may live in another mount namespace: open its file through its root.
  std::string buildId = readFile("/proc/" + std::to_string(pid_) + "/root" + mapping.path);
  if (buildId.empty()) {
    char range[64];
    std::snprintf(range, sizeof range, "%" PRIx64 "-%" PRIx64, mapping.start, mapping.end);
    buildId = readFile("/proc/" + std::to_string(pid_) + "/map_files/" + range);
  }
  return cache_.emplace(mapping.path, std::move(buildId)).first->second;
}

std::string BuildIdReader::readFile(const std::string &path) const {
  const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0) return {};
  std::string buildId;
  Elf64_Ehdr elf;
  if (readAt(fd, &elf, sizeof elf, 0) && std::memcmp(elf.e_ident, ELFMAG, SELFMAG) == 0 &&
      elf.e_ident[EI_CLASS] == ELFCLASS64 && elf.e_phentsize == sizeof(Elf64_Phdr)) {
    for (int i = 0; i < elf.e_phnum && buildId.empty(); ++i) {
      Elf64_Phdr segment;
      if (!readAt(fd, &segment, sizeof segment, static_cast<off_t>(elf.e_phoff + i * sizeof segment))) break;
      if (segment.p_type != PT_NOTE || segment.p_filesz == 0 || segment.p_filesz > kMaximumNoteSegment) continue;
      std::vector<std::uint8_t> notes(segment.p_filesz);
      if (readAt(fd, notes.data(), notes.size(), static_cast<off_t>(segment.p_offset))) buildId = buildIdOfNotes(notes);
    }
  }
  close(fd);
  return buildId;
}

}  // namespace stack_sampler
