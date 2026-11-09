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

#include "cPerfEvents.hpp"

#include <linux/perf_event.h>
#include <poll.h>
#include <unistd.h>

#include <cmath>
#include <cstring>

namespace stack_sampler {

namespace {

// 128 KiB per CPU: at 99 Hz a ring holds seconds of samples between drains.
constexpr std::size_t kDataPages = 32;

}  // namespace

std::string PerfEvents::open(double frequency) {
  perf_event_attr attribute;
  std::memset(&attribute, 0, sizeof attribute);
  attribute.size = sizeof attribute;
  attribute.type = PERF_TYPE_SOFTWARE;
  attribute.config = PERF_COUNT_SW_CPU_CLOCK;
  attribute.freq = 1;
  attribute.sample_freq = static_cast<std::uint64_t>(std::llround(frequency));
  attribute.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_CALLCHAIN;
  attribute.exclude_callchain_kernel = 1;
  attribute.disabled = 1;
  attribute.watermark = 1;
  attribute.wakeup_watermark = static_cast<std::uint32_t>(kDataPages * static_cast<std::size_t>(sysconf(_SC_PAGESIZE)) / 4);

  const long configured = sysconf(_SC_NPROCESSORS_CONF);
  std::string firstError;
  for (int cpu = 0; cpu < configured; ++cpu) {
    PerfRing ring;
    const std::string error = ring.open(attribute, cpu, kDataPages);
    if (!error.empty()) {
      if (firstError.empty()) firstError = error;  // an offline CPU fails as well
      continue;
    }
    rings_.push_back(std::move(ring));
  }
  return rings_.empty() ? (firstError.empty() ? std::string("no CPUs") : firstError) : std::string();
}

void PerfEvents::enable() {
  for (const PerfRing &ring : rings_) ring.enable();
}

void PerfEvents::disable() {
  for (const PerfRing &ring : rings_) ring.disable();
}

void PerfEvents::wait(int timeoutMilliseconds) const {
  std::vector<pollfd> fds;
  fds.reserve(rings_.size());
  for (const PerfRing &ring : rings_) fds.push_back(pollfd{ring.fd(), POLLIN, 0});
  poll(fds.data(), static_cast<nfds_t>(fds.size()), timeoutMilliseconds);
}

void PerfEvents::drain(const std::function<void(const PerfSample &)> &handler) {
  for (PerfRing &ring : rings_) ring.drain(handler);
}

std::uint64_t PerfEvents::lost() const {
  std::uint64_t lost = 0;
  for (const PerfRing &ring : rings_) lost += ring.lost();
  return lost;
}

}  // namespace stack_sampler
