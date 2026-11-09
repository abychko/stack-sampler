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

enum class ExitCode {
  // The profile is written, or --help or --version.
  Success = 0,

  // The command line is invalid.
  UsageError = 1,

  // The target process cannot be sampled: it does not exist, its /proc files
  // cannot be read (run as root), or the platform is not supported.
  TargetError = 2,

  // The profile cannot be written to the output file.
  OutputError = 3,
};

}  // namespace stack_sampler
