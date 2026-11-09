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

#include "cWaitSampler.hpp"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>

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

WaitSampler::WaitSampler(int pid, const MemoryMap &map, ProfileBuilder &profile, std::int64_t wallPerSample)
    : map_(map),
      profile_(profile),
      threads_(pid),
      memory_(pid),
      descriptors_(pid),
      classifier_(memory_, descriptors_),
      code_(map_, memory_),
      walker_(code_),
      wall_(wallPerSample) {}

std::int64_t WaitSampler::wallPerSample(double frequency) { return std::llround(1e9 / frequency); }

void WaitSampler::pass(SamplerStatistics &statistics) {
  ++statistics.passes;
  for (int tid : threads_.threads()) sampleThread(tid, statistics);
}

void WaitSampler::sampleThread(int tid, SamplerStatistics &statistics) {
  ++statistics.threads;
  const std::optional<ThreadStatus> status = threads_.status(tid);
  const SyscallState before = threads_.syscall(tid);
  if (!status || before.status == SyscallStatus::Unknown) {
    ++statistics.gone;
    return;
  }

  SampleLabels labels;
  labels.strings = {{"profile", "wait"}, {"thread", status->name}, {"state", std::string(1, status->state)}};
  labels.numbers = {{"tid", tid}};
  const std::int64_t values[] = {1, wall_, 0};  // samples, wall, cpu

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

}  // namespace stack_sampler
