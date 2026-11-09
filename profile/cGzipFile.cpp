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

#include "cGzipFile.hpp"

#include <zlib.h>

#include <algorithm>

namespace stack_sampler {

std::string GzipFile::write(const std::string &path, std::string_view bytes) {
  gzFile file = gzopen(path.c_str(), "wb");
  if (file == nullptr) return "cannot open " + path;
  std::size_t written = 0;
  while (written < bytes.size()) {
    const auto chunk = static_cast<unsigned>(std::min<std::size_t>(bytes.size() - written, 1U << 30));
    const int done = gzwrite(file, bytes.data() + written, chunk);
    if (done <= 0) {
      gzclose(file);
      return "cannot write " + path;
    }
    written += static_cast<std::size_t>(done);
  }
  if (gzclose(file) != Z_OK) return "cannot close " + path;
  return {};
}

}  // namespace stack_sampler
