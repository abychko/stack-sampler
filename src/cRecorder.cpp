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

#include "cRecorder.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

namespace stack_sampler {

namespace {

using Clock = std::chrono::steady_clock;

// Reading the map takes the memory-map lock of the target: it is reread seldom,
// and at once when a wait pass meets a stack it lacks.
constexpr auto kMapRefresh = std::chrono::seconds(10);
constexpr auto kLongestSleep = std::chrono::milliseconds(100);

Clock::duration seconds(double value) {
  return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(value));
}

}  // namespace

Recorder::Recorder(const Options &options, ProfileBuilder &profile)
    : options_(options), profile_(profile), map_(options.pid), buildIds_(options.pid) {
  if (options.waitProfile)
    wait_.emplace(options.pid, map_, profile_, WaitSampler::wallPerSample(options.frequency),
                  [this] { return loadMap(); });
  if (options.cpuProfile) cpu_.emplace(options.pid, profile_, options.cpuFrequency);
}

std::string Recorder::start() {
  if (!loadMap()) return "cannot read the memory map of the process";
  return cpu_ ? cpu_->start() : std::string();
}

SamplerStatistics Recorder::run(const std::atomic<bool> &stopRequested) {
  SamplerStatistics statistics;
  const auto deadline = Clock::now() + seconds(options_.duration);
  const auto period = seconds(1.0 / options_.frequency);
  auto nextPass = Clock::now();
  auto nextMap = Clock::now() + kMapRefresh;
  bool gone = false;
  while (!stopRequested.load() && !gone && Clock::now() < deadline) {
    if (wait_ && Clock::now() >= nextPass) {
      const auto started = Clock::now();
      if (!wait_->pass(statistics)) break;  // the target exited
      statistics.passMilliseconds.push_back(std::chrono::duration<double, std::milli>(Clock::now() - started).count());
      nextPass += period;
      if (nextPass < Clock::now()) {
        ++statistics.latePasses;
        nextPass = Clock::now();
      }
    }
    if (Clock::now() >= nextMap) {
      gone = !loadMap();
      nextMap = Clock::now() + kMapRefresh;
    }
    if (cpu_) cpu_->drain(statistics);

    auto until = std::min(deadline, nextMap);
    if (wait_) until = std::min(until, nextPass);
    const auto pause = std::clamp<Clock::duration>(until - Clock::now(), Clock::duration::zero(), kLongestSleep);
    if (cpu_)
      cpu_->wait(static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(pause).count()));
    else
      std::this_thread::sleep_for(pause);
  }
  if (cpu_) cpu_->stop(statistics);
  return statistics;
}

// Rereads the memory map and adds its executable file mappings, with build ids,
// to the profile.
bool Recorder::loadMap() {
  if (!map_.load()) return false;
  for (const Mapping &mapping : map_.mappings()) {
    if (!mapping.executable || mapping.path.empty()) continue;
    ProfileMapping recorded{mapping.start, mapping.end, mapping.offset, mapping.path, {}};
    if (mapping.path.front() == '/') recorded.buildId = buildIds_.read(mapping);
    profile_.addMapping(recorded);
  }
  return true;
}

}  // namespace stack_sampler
