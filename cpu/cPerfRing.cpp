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

#include "cPerfRing.hpp"

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

namespace stack_sampler {

namespace {

long perfEventOpen(perf_event_attr *attribute, int pid, int cpu, int group, unsigned long flags) {
  return syscall(SYS_perf_event_open, attribute, pid, cpu, group, flags);
}

template <typename T>
T readField(const std::uint8_t *&at) {
  T value;
  std::memcpy(&value, at, sizeof value);
  at += sizeof value;
  return value;
}

}  // namespace

PerfRing::PerfRing(PerfRing &&other) noexcept
    : fd_(other.fd_),
      mapping_(other.mapping_),
      mappingSize_(other.mappingSize_),
      dataSize_(other.dataSize_),
      lost_(other.lost_) {
  other.fd_ = -1;
  other.mapping_ = nullptr;
}

PerfRing::~PerfRing() {
  if (mapping_ != nullptr) munmap(mapping_, mappingSize_);
  if (fd_ >= 0) close(fd_);
}

std::string PerfRing::open(perf_event_attr &attribute, int cpu, std::size_t dataPages) {
  const long fd = perfEventOpen(&attribute, -1, cpu, -1, PERF_FLAG_FD_CLOEXEC);
  if (fd < 0) return std::string("perf_event_open: ") + std::strerror(errno);
  fd_ = static_cast<int>(fd);
  const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
  dataSize_ = dataPages * page;
  mappingSize_ = dataSize_ + page;
  void *mapping = mmap(nullptr, mappingSize_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (mapping == MAP_FAILED) return std::string("mmap of the perf ring: ") + std::strerror(errno);
  mapping_ = mapping;
  return {};
}

bool PerfRing::enable() const { return ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0) == 0; }

bool PerfRing::disable() const { return ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0) == 0; }

void PerfRing::drain(const std::function<void(const PerfSample &)> &handler) {
  if (mapping_ == nullptr) return;
  auto *control = static_cast<perf_event_mmap_page *>(mapping_);
  const auto *data = static_cast<const std::uint8_t *>(mapping_) + (mappingSize_ - dataSize_);
  const std::uint64_t head = __atomic_load_n(&control->data_head, __ATOMIC_ACQUIRE);
  std::uint64_t tail = control->data_tail;
  while (tail + sizeof(perf_event_header) <= head) {
    perf_event_header header;
    for (std::size_t i = 0; i < sizeof header; ++i)
      reinterpret_cast<std::uint8_t *>(&header)[i] = data[(tail + i) % dataSize_];
    if (header.size < sizeof header || tail + header.size > head) break;
    // A record may wrap around the end of the ring: copy it out first.
    record_.resize(header.size);
    const std::size_t start = tail % dataSize_;
    const std::size_t first = std::min<std::size_t>(header.size, dataSize_ - start);
    std::memcpy(record_.data(), data + start, first);
    std::memcpy(record_.data() + first, data, header.size - first);

    const std::uint8_t *body = record_.data() + sizeof header;
    const std::size_t bodySize = header.size - sizeof header;
    if (header.type == PERF_RECORD_SAMPLE) {
      parseSample(body, bodySize, header.misc, handler);
    } else if (header.type == PERF_RECORD_LOST && bodySize >= 2 * sizeof(std::uint64_t)) {
      const std::uint8_t *at = body + sizeof(std::uint64_t);
      lost_ += readField<std::uint64_t>(at);
    }
    tail += header.size;
  }
  __atomic_store_n(&control->data_tail, tail, __ATOMIC_RELEASE);
}

// Layout for PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_PERIOD | PERF_SAMPLE_CALLCHAIN,
// in the kernel's order: ip, pid, tid, period, nr, ips[nr].
void PerfRing::parseSample(const std::uint8_t *record, std::size_t size, std::uint16_t misc,
                           const std::function<void(const PerfSample &)> &handler) {
  constexpr std::size_t kFixed = 8 + 4 + 4 + 8 + 8;
  if (size < kFixed) return;
  const std::uint8_t *at = record;
  readField<std::uint64_t>(at);  // ip: the user one is the first user callchain entry
  sample_.pid = static_cast<int>(readField<std::uint32_t>(at));
  sample_.tid = static_cast<int>(readField<std::uint32_t>(at));
  sample_.period = readField<std::uint64_t>(at);
  const auto count = readField<std::uint64_t>(at);
  if (count > (size - kFixed) / sizeof(std::uint64_t)) return;
  sample_.kernel = (misc & PERF_RECORD_MISC_CPUMODE_MASK) == PERF_RECORD_MISC_KERNEL;
  sample_.userFrames.clear();
  bool user = false;
  for (std::uint64_t i = 0; i < count; ++i) {
    const auto address = readField<std::uint64_t>(at);
    if (address >= static_cast<std::uint64_t>(PERF_CONTEXT_MAX)) {
      user = address == static_cast<std::uint64_t>(PERF_CONTEXT_USER);
      continue;
    }
    if (user) sample_.userFrames.push_back(address);
  }
  handler(sample_);
}

}  // namespace stack_sampler
