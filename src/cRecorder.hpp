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

#include <atomic>
#include <optional>
#include <string>

#include "cBuildIdReader.hpp"
#include "cCpuSampler.hpp"
#include "cMemoryMap.hpp"
#include "cProfileBuilder.hpp"
#include "cWaitSampler.hpp"
#include "sOptions.hpp"
#include "sSamplerStatistics.hpp"

namespace stack_sampler {

// Runs the chosen profiles together for the duration: wait passes at their
// frequency, CPU rings drained in between, the memory map of the target reread
// before every pass and at least once a second for the mappings of the profile.
class Recorder {
 public:
  Recorder(const Options &options, ProfileBuilder &profile);

  // Empty on success, else why recording cannot start.
  std::string start();
  SamplerStatistics run(const std::atomic<bool> &stopRequested);

 private:
  bool loadMap();

  const Options &options_;
  ProfileBuilder &profile_;
  MemoryMap map_;
  BuildIdReader buildIds_;
  std::optional<WaitSampler> wait_;
  std::optional<CpuSampler> cpu_;
};

}  // namespace stack_sampler
