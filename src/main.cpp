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

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <string>

#include "cCommandLine.hpp"
#include "eExitCode.hpp"

#ifdef __linux__
#include <algorithm>
#include <cstdint>
#include <vector>

#include "cGzipFile.hpp"
#include "cProfileBuilder.hpp"
#include "cSampler.hpp"
#include "cThreadReader.hpp"
#endif

namespace {

using stack_sampler::ExitCode;

void printUsage(std::FILE *stream) {
  std::fprintf(stream,
               "Usage: %s --pid PID --output FILE [--frequency HZ] [--duration SECONDS]\n"
               "       %s --help | --version\n"
               "\n"
               "Takes the wait profile of a running process without stopping it and\n"
               "writes it as a gzip-compressed pprof profile with raw addresses.\n"
               "\n"
               "  --pid PID           process to sample\n"
               "  --output FILE       pprof profile to write\n"
               "  --frequency HZ      passes over all threads per second, default 1\n"
               "  --duration SECONDS  how long to sample, default 10; SIGINT stops earlier\n",
               STACK_SAMPLER_NAME, STACK_SAMPLER_NAME);
}

int exitWith(ExitCode code) { return static_cast<int>(code); }

#ifdef __linux__
std::atomic<bool> stopRequested{false};

void requestStop(int) { stopRequested.store(true); }

int sample(const stack_sampler::Options &options) {
  using namespace stack_sampler;
  if (ThreadReader(options.pid).syscall(options.pid).status == SyscallStatus::Unknown) {
    std::fprintf(stderr, "%s: cannot read /proc/%d/syscall: no such process, or not root\n", STACK_SAMPLER_NAME,
                 options.pid);
    return exitWith(ExitCode::TargetError);
  }
  std::signal(SIGINT, requestStop);
  std::signal(SIGTERM, requestStop);

  const std::int64_t wall = Sampler::wallPerSample(options.frequency);
  ProfileBuilder profile({{"samples", "count"}, {"wall", "nanoseconds"}}, {"wall", "nanoseconds"}, wall);
  Sampler sampler(options, profile);
  const auto started = std::chrono::system_clock::now();
  const auto startedSteady = std::chrono::steady_clock::now();
  SamplerStatistics statistics = sampler.run(stopRequested);
  const auto elapsed = std::chrono::steady_clock::now() - startedSteady;

  profile.setTime(std::chrono::duration_cast<std::chrono::nanoseconds>(started.time_since_epoch()).count(),
                  std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
  profile.addComment(std::string(STACK_SAMPLER_NAME) + " " + STACK_SAMPLER_VERSION + " (commit " +
                     STACK_SAMPLER_COMMIT + ")");
  profile.addComment("pid " + std::to_string(options.pid) + ", " + std::to_string(options.frequency) + " Hz");
  const std::size_t profileSamples = profile.sampleCount();
  const std::string error = GzipFile::write(options.output, profile.serialize());
  if (!error.empty()) {
    std::fprintf(stderr, "%s: %s\n", STACK_SAMPLER_NAME, error.c_str());
    return exitWith(ExitCode::OutputError);
  }

  std::vector<double> &times = statistics.passMilliseconds;
  std::sort(times.begin(), times.end());
  const double median = times.empty() ? 0 : times[times.size() / 2];
  const double longest = times.empty() ? 0 : times.back();
  std::fprintf(stderr,
               "passes=%llu late=%llu threads=%llu blocked=%llu running=%llu moved=%llu no_stack=%llu "
               "leaf_callers=%llu gone=%llu profile_samples=%zu pass_ms_median=%.1f pass_ms_max=%.1f\n",
               static_cast<unsigned long long>(statistics.passes), static_cast<unsigned long long>(statistics.latePasses),
               static_cast<unsigned long long>(statistics.threads), static_cast<unsigned long long>(statistics.blocked),
               static_cast<unsigned long long>(statistics.running), static_cast<unsigned long long>(statistics.moved),
               static_cast<unsigned long long>(statistics.noStack),
               static_cast<unsigned long long>(statistics.leafCallers), static_cast<unsigned long long>(statistics.gone),
               profileSamples, median, longest);
  return exitWith(ExitCode::Success);
}
#endif

}  // namespace

int main(int argc, char **argv) {
  std::string error;
  const auto options = stack_sampler::CommandLine::parse(argc, argv, error);
  if (!options) {
    std::fprintf(stderr, "%s: %s\n", STACK_SAMPLER_NAME, error.c_str());
    printUsage(stderr);
    return exitWith(ExitCode::UsageError);
  }
  if (options->help) {
    printUsage(stdout);
    return exitWith(ExitCode::Success);
  }
  if (options->version) {
    std::printf("%s %s (commit %s)\n", STACK_SAMPLER_NAME, STACK_SAMPLER_VERSION, STACK_SAMPLER_COMMIT);
    return exitWith(ExitCode::Success);
  }
#ifdef __linux__
  return sample(*options);
#else
  std::fprintf(stderr, "%s: sampling is supported on Linux only\n", STACK_SAMPLER_NAME);
  return exitWith(ExitCode::TargetError);
#endif
}
