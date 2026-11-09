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

#include "cCommandLine.hpp"

#include <algorithm>
#include <cstdlib>
#include <string_view>

namespace stack_sampler {

namespace {

constexpr double kMinimumFrequency = 0.01;
constexpr double kMaximumFrequency = 100.0;
constexpr double kMaximumDuration = 7 * 24 * 3600.0;
constexpr double kMaximumCpuFrequency = 1000.0;

bool parseNumber(const std::string &text, double &value) {
  char *end = nullptr;
  value = std::strtod(text.c_str(), &end);
  return !text.empty() && end != nullptr && *end == '\0';
}

// "wait", "cpu" or both, comma-separated.
bool parseProfiles(const std::string &text, Options &options) {
  options.waitProfile = options.cpuProfile = false;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t comma = std::min(text.find(',', start), text.size());
    const std::string_view kind(text.data() + start, comma - start);
    if (kind == "wait")
      options.waitProfile = true;
    else if (kind == "cpu")
      options.cpuProfile = true;
    else
      return false;
    start = comma + 1;
  }
  return options.waitProfile || options.cpuProfile;
}

}  // namespace

std::optional<Options> CommandLine::parse(int argc, char **argv, std::string &error) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    std::string name(argv[i]);
    std::string value;
    bool hasValue = false;
    if (const std::size_t equals = name.find('='); equals != std::string::npos) {
      value = name.substr(equals + 1);
      name.resize(equals);
      hasValue = true;
    }
    if (name == "--help" || name == "--version") {
      (name == "--help" ? options.help : options.version) = true;
      continue;
    }
    if (name != "--pid" && name != "--frequency" && name != "--duration" && name != "--output" &&
        name != "--profile" && name != "--cpu-frequency") {
      error = "unknown option " + name;
      return std::nullopt;
    }
    if (!hasValue) {
      if (i + 1 >= argc) {
        error = name + " needs a value";
        return std::nullopt;
      }
      value = argv[++i];
    }
    double number = 0;
    if (name == "--output") {
      options.output = value;
    } else if (name == "--profile") {
      if (!parseProfiles(value, options)) {
        error = "--profile: expected wait, cpu or wait,cpu: " + value;
        return std::nullopt;
      }
    } else if (!parseNumber(value, number)) {
      error = name + ": not a number: " + value;
      return std::nullopt;
    } else if (name == "--pid") {
      if (number < 1 || number != static_cast<int>(number)) {
        error = "--pid: not a process id: " + value;
        return std::nullopt;
      }
      options.pid = static_cast<int>(number);
    } else if (name == "--frequency") {
      if (number < kMinimumFrequency || number > kMaximumFrequency) {
        error = "--frequency: out of range 0.01..100: " + value;
        return std::nullopt;
      }
      options.frequency = number;
    } else if (name == "--cpu-frequency") {
      if (number < 1 || number > kMaximumCpuFrequency) {
        error = "--cpu-frequency: out of range 1..1000: " + value;
        return std::nullopt;
      }
      options.cpuFrequency = number;
    } else {
      if (number <= 0 || number > kMaximumDuration) {
        error = "--duration: out of range (0, 604800]: " + value;
        return std::nullopt;
      }
      options.duration = number;
    }
  }
  if (options.help || options.version) return options;
  if (options.pid == 0 || options.output.empty()) {
    error = "--pid and --output are required";
    return std::nullopt;
  }
  return options;
}

}  // namespace stack_sampler
