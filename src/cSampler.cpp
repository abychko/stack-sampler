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

#include "cSampler.hpp"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <thread>

namespace stack_sampler {

namespace {

// The copy stops at the end of the stack mapping anyway.
constexpr std::uint64_t kMaximumStackCopy = 512 * 1024;

std::string hex(std::uint64_t value) {
  char text[24];
  std::snprintf(text, sizeof text, "0x%" PRIx64, value);
  return text;
}

}  // namespace

Sampler::Sampler(const Options &options, ProfileBuilder &profile)
    : options_(options),
      profile_(profile),
      threads_(options.pid),
      map_(options.pid),
      memory_(options.pid),
      buildIds_(options.pid),
      descriptors_(options.pid),
      classifier_(memory_, descriptors_),
      code_(map_, memory_),
      walker_(code_),
      wall_(wallPerSample(options.frequency)) {}

std::int64_t Sampler::wallPerSample(double frequency) { return std::llround(1e9 / frequency); }

SamplerStatistics Sampler::run(const std::atomic<bool> &stopRequested) {
  using Clock = std::chrono::steady_clock;
  SamplerStatistics statistics;
  const auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / options_.frequency));
  const auto deadline = Clock::now() + std::chrono::duration_cast<Clock::duration>(
                                           std::chrono::duration<double>(options_.duration));
  auto next = Clock::now();
  while (!stopRequested.load() && Clock::now() < deadline) {
    const auto started = Clock::now();
    if (!pass(statistics)) break;  // the target exited
    statistics.passMilliseconds.push_back(std::chrono::duration<double, std::milli>(Clock::now() - started).count());
    next += period;
    if (next < Clock::now()) {
      ++statistics.latePasses;
      next = Clock::now();
    }
    // Sleep in short steps so that a stop request is seen soon.
    while (!stopRequested.load() && Clock::now() < std::min(next, deadline))
      std::this_thread::sleep_for(std::min<Clock::duration>(std::min(next, deadline) - Clock::now(),
                                                            std::chrono::milliseconds(100)));
  }
  return statistics;
}

bool Sampler::pass(SamplerStatistics &statistics) {
  if (!map_.load()) return false;
  ++statistics.passes;
  addMappings();
  for (int tid : threads_.threads()) sampleThread(tid, statistics);
  return true;
}

void Sampler::sampleThread(int tid, SamplerStatistics &statistics) {
  ++statistics.threads;
  const std::optional<ThreadStatus> status = threads_.status(tid);
  const SyscallState before = threads_.syscall(tid);
  if (!status || before.status == SyscallStatus::Unknown) {
    ++statistics.gone;
    return;
  }

  SampleLabels labels;
  labels.strings = {{"thread", status->name}, {"state", std::string(1, status->state)}};
  labels.numbers = {{"tid", tid}};
  const std::int64_t values[] = {1, wall_};

  if (before.status == SyscallStatus::Running) {
    ++statistics.running;
    labels.strings.push_back({"wait", "running"});
    profile_.addSample({}, labels, values);
    return;
  }

  StackTrace trace;
  if (const Mapping *stackMapping = map_.find(before.stackPointer)) {
    stack_.base = before.stackPointer & ~std::uint64_t{7};
    stack_.bytes.resize(std::min(stackMapping->end - stack_.base, kMaximumStackCopy));
    stack_.bytes.resize(memory_.read(stack_.base, stack_.bytes));
    trace = walker_.walk(stack_, before.stackPointer, before.programCounter);
  }
  const SyscallState after = threads_.syscall(tid);
  if (after.raw != before.raw) {
    ++statistics.moved;
    return;
  }
  ++statistics.blocked;
  if (!trace.chainFound) ++statistics.noStack;
  if (trace.leafCallerAdded) ++statistics.leafCallers;
  if (trace.frames.empty()) trace.frames.push_back(before.programCounter);

  const WaitDescription wait = classifier_.classify(before);
  labels.strings.push_back({"wait", wait.waitClass});
  labels.strings.push_back({"syscall", wait.syscall});
  if (wait.futex) labels.strings.push_back({"futex", hex(*wait.futex)});
  if (wait.fdKind) labels.strings.push_back({"fd", *wait.fdKind});
  profile_.addSample(trace.frames, labels, values);
}

void Sampler::addMappings() {
  for (const Mapping &mapping : map_.mappings()) {
    if (!mapping.executable || mapping.path.empty()) continue;
    ProfileMapping recorded{mapping.start, mapping.end, mapping.offset, mapping.path, {}};
    if (mapping.path.front() == '/') recorded.buildId = buildIds_.read(mapping);
    profile_.addMapping(recorded);
  }
}

}  // namespace stack_sampler
