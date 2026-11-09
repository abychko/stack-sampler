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

#include "cCpuSampler.hpp"

namespace stack_sampler {

std::string CpuSampler::start() {
  const std::string error = events_.open(frequency_);
  if (error.empty()) events_.enable();
  return error;
}

void CpuSampler::drain(SamplerStatistics &statistics) {
  events_.drain([&](const PerfSample &sample) {
    if (sample.pid != pid_) {
      ++statistics.cpuOtherProcesses;
      return;
    }
    ++statistics.cpuSamples;
    frames_.clear();
    for (std::size_t i = 0; i < sample.userFrames.size(); ++i)
      frames_.push_back(i == 0 ? sample.userFrames[i] : sample.userFrames[i] - 1);  // callers point into the call
    if (frames_.empty()) ++statistics.cpuNoStack;

    SampleLabels labels;
    labels.strings = {{"profile", "cpu"}, {"thread", threadName(sample.tid)}, {"mode", sample.kernel ? "kernel" : "user"}};
    labels.numbers = {{"tid", sample.tid}};
    const std::int64_t values[] = {1, 0, static_cast<std::int64_t>(sample.period)};  // samples, wall, cpu
    profile_.addSample(frames_, labels, values);
  });
}

void CpuSampler::stop(SamplerStatistics &statistics) {
  events_.disable();
  drain(statistics);
  statistics.cpuLost = events_.lost();
  statistics.cpus = events_.cpus();
}

const std::string &CpuSampler::threadName(int tid) {
  const auto found = names_.find(tid);
  if (found != names_.end()) return found->second;
  const auto status = threads_.status(tid);
  return names_.emplace(tid, status ? status->name : std::string("?")).first->second;
}

}  // namespace stack_sampler
