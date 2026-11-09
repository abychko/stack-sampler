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

#include <cstdio>
#include <string_view>

#include "eExitCode.hpp"

namespace {

using stack_sampler::ExitCode;

void printUsage(std::FILE *stream) {
  std::fprintf(stream, "Usage: %s [--version] [--help]\n", STACK_SAMPLER_NAME);
}

int exitWith(ExitCode code) { return static_cast<int>(code); }

}  // namespace

int main(int argc, char **argv) {
  if (argc == 2) {
    const std::string_view argument(argv[1]);
    if (argument == "--version") {
      std::printf("%s %s\n", STACK_SAMPLER_NAME, STACK_SAMPLER_VERSION);
      return exitWith(ExitCode::Success);
    }
    if (argument == "--help") {
      printUsage(stdout);
      return exitWith(ExitCode::Success);
    }
  }
  printUsage(stderr);
  return exitWith(ExitCode::UsageError);
}
