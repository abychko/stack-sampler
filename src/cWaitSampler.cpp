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
#include <cstring>
#include <string_view>

namespace stack_sampler {

namespace {

constexpr std::uint64_t kFirstStackCopy = 4 * 1024;
constexpr std::uint64_t kMaximumStackCopy = 512 * 1024;

// The frame of a thread that is not blocked: it has no stack in this profile.
constexpr std::string_view kRunningFrame = "[running]";

// The state letter of /proc/<tid>/stat for task->__state: a thread that is being
// woken counts as running.
char stateLetter(unsigned int state) {
  constexpr unsigned int kReported = 0x7f, kIdle = 0x402, kLockWait = 0x1000;
  constexpr char kLetters[] = "RSDTtXZP";
  if ((state & kIdle) == kIdle) return 'I';
  if (state & kLockWait) return 'D';
  unsigned int index = 0;
  for (unsigned int reported = state & kReported; reported != 0; reported >>= 1) ++index;
  return kLetters[index];
}

std::string hex(std::uint64_t value) {
  char text[24];
  std::snprintf(text, sizeof text, "0x%" PRIx64, value);
  return text;
}

}  // namespace

WaitSampler::WaitSampler(int pid, const MemoryMap &map, ProfileBuilder &profile, std::int64_t wallPerSample,
                         std::function<bool()> reloadMap)
    : map_(map),
      profile_(profile),
      reloadMap_(std::move(reloadMap)),
      threads_(pid),
      memory_(pid),
      descriptors_(pid),
      classifier_(memory_, descriptors_),
      code_(map_, memory_),
      walker_(code_),
      wall_(wallPerSample) {
#ifdef STACK_SAMPLER_HAS_BPF
  tasks_.emplace();
  if (const std::string error = tasks_->open(pid); !error.empty()) {
    std::fprintf(stderr, "stack-sampler: %s; reading /proc instead\n", error.c_str());
    tasks_.reset();
  }
#endif
}

std::int64_t WaitSampler::wallPerSample(double frequency) { return std::llround(1e9 / frequency); }

bool WaitSampler::pass(SamplerStatistics &statistics) {
#ifdef STACK_SAMPLER_HAS_BPF
  if (tasks_) {
    statistics.taskIterator = true;
    if (!tasks_->pass(records_) || records_.empty()) return false;
    ++statistics.passes;
    descriptors_.newPass();
    mapReloaded_ = false;
    for (const ssr_task_record &record : records_) sampleRecord(record, statistics);
    return true;
  }
#endif
  const std::vector<int> tids = threads_.threads();
  if (tids.empty()) return false;
  ++statistics.passes;
  descriptors_.newPass();
  mapReloaded_ = false;
  for (int tid : tids) sampleThread(tid, statistics);
  return true;
}

// Copies the stack from the stack pointer and walks it; a chain that runs past a
// short copy is walked again on a copy up to the end of the stack mapping.
StackTrace WaitSampler::walkStack(const Mapping &stackMapping, const SyscallState &state,
                                  SamplerStatistics &statistics) {
  stack_.base = state.stackPointer & ~std::uint64_t{7};
  const std::uint64_t live = std::min(stackMapping.end - stack_.base, kMaximumStackCopy);
  stack_.bytes.resize(std::min(live, kFirstStackCopy));
  stack_.bytes.resize(memory_.read(stack_.base, stack_.bytes));
  StackTrace trace = walker_.walk(stack_, state.stackPointer, state.programCounter);
  if ((trace.truncated || !trace.chainFound) && stack_.bytes.size() < live) {
    ++statistics.stackRecopied;
    stack_.bytes.resize(live);
    stack_.bytes.resize(memory_.read(stack_.base, stack_.bytes));
    trace = walker_.walk(stack_, state.stackPointer, state.programCounter);
  }
  return trace;
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
    profile_.addSample(kRunningFrame, labels, values);
    return;
  }

  const Mapping *stackMapping = stackMappingOf(before.stackPointer);
  StackTrace trace;
  if (stackMapping != nullptr) trace = walkStack(*stackMapping, before, statistics);
  if (!threads_.syscallUnchanged(tid, before.raw)) {
    ++statistics.moved;
    return;
  }
  ++statistics.blocked;
  if (!trace.chainFound) ++statistics.noStack;
  if (trace.leafCallerAdded) ++statistics.leafCallers;
  if (trace.frames.empty()) trace.frames.push_back(before.programCounter);
  addBlocked(labels, before, std::move(trace.frames));
}

const Mapping *WaitSampler::stackMappingOf(std::uint64_t stackPointer) {
  const Mapping *mapping = map_.find(stackPointer);
  if (mapping == nullptr && !mapReloaded_ && reloadMap_) {
    // A thread started after the map was read: its stack is not in it yet.
    mapReloaded_ = true;
    reloadMap_();
    mapping = map_.find(stackPointer);
  }
  return mapping;
}

void WaitSampler::addBlocked(SampleLabels &labels, const SyscallState &state, std::vector<std::uint64_t> frames) {
  const std::int64_t values[] = {1, wall_, 0};  // samples, wall, cpu
  const WaitDescription wait = classifier_.classify(state);
  labels.strings.push_back({"wait", wait.waitClass});
  labels.strings.push_back({"syscall", wait.syscall});
  if (wait.futex) labels.strings.push_back({"futex", hex(*wait.futex)});
  if (wait.fdKind) labels.strings.push_back({"fd", *wait.fdKind});
  profile_.addSample(frames, labels, values);
}

#ifdef STACK_SAMPLER_HAS_BPF
// A thread from the task iterator: registers and the chain from RBP come from
// the kernel; the caller of a frameless leaf is taken from [sp], and a chain
// broken by code without frame pointers is searched from SP as on /proc.
void WaitSampler::sampleRecord(const ssr_task_record &record, SamplerStatistics &statistics) {
  ++statistics.threads;
  const char state = stateLetter(record.state);
  SampleLabels labels;
  labels.strings = {{"profile", "wait"}, {"thread", std::string(record.name, strnlen(record.name, sizeof record.name))},
                    {"state", std::string(1, state)}};
  labels.numbers = {{"tid", static_cast<std::int64_t>(record.tid)}};
  if (state == 'R') {
    ++statistics.running;
    labels.strings.push_back({"wait", "running"});
    const std::int64_t values[] = {1, wall_, 0};
    profile_.addSample(kRunningFrame, labels, values);
    return;
  }

  SyscallState syscall;
  syscall.status = record.syscall >= 0 ? SyscallStatus::InSyscall : SyscallStatus::NotInSyscall;
  syscall.number = record.syscall;
  syscall.arguments[0] = record.arg0;
  syscall.arguments[1] = record.arg1;
  syscall.stackPointer = record.sp;
  syscall.programCounter = record.ip;

  std::vector<std::uint64_t> frames{record.ip};
  const std::uint32_t count = std::min<std::uint32_t>(record.frames, SSR_TASK_FRAMES);
  if (!record.complete) {
    // Code without frame pointers broke the chain: search from SP instead.
    if (const Mapping *stackMapping = stackMappingOf(record.sp)) {
      StackTrace trace = walkStack(*stackMapping, syscall, statistics);
      if (trace.chainFound && trace.frames.size() > count + 1) {
        ++statistics.searchedFromStackPointer;
        if (trace.leafCallerAdded) ++statistics.leafCallers;
        ++statistics.blocked;
        addBlocked(labels, syscall, std::move(trace.frames));
        return;
      }
    }
    if (count == 0) ++statistics.noStack;
  }
  if (code_.isCode(record.at_sp) && walker_.followsCall(record.at_sp) &&
      (count == 0 || record.at_sp != record.ra[0])) {
    frames.push_back(record.at_sp - 1);
    ++statistics.leafCallers;
  }
  for (std::uint32_t i = 0; i < count && code_.isCode(record.ra[i]); ++i) frames.push_back(record.ra[i] - 1);
  ++statistics.blocked;
  addBlocked(labels, syscall, std::move(frames));
}
#endif

}  // namespace stack_sampler
