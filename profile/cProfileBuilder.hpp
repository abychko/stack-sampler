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

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sProfileMapping.hpp"
#include "sSampleLabels.hpp"

namespace stack_sampler {

// Collects samples with raw addresses and labels and serializes them as a pprof
// profile (profile.proto). Equal samples (same addresses and labels) are merged
// by adding their values.
class ProfileBuilder {
 public:
  using ValueType = std::pair<std::string, std::string>;  // type, unit

  ProfileBuilder(std::vector<ValueType> sampleTypes, ValueType periodType, std::int64_t period);

  void addMapping(const ProfileMapping &mapping);
  void addSample(std::span<const std::uint64_t> addresses, const SampleLabels &labels,
                 std::span<const std::int64_t> values);
  void setTime(std::int64_t startNanoseconds, std::int64_t durationNanoseconds);
  void addComment(std::string comment);
  void setDefaultSampleType(std::string type) { defaultSampleType_ = std::move(type); }

  std::size_t sampleCount() const { return samples_.size(); }
  std::string serialize();

 private:
  struct Sample {
    std::vector<std::uint64_t> locations;
    SampleLabels labels;
    std::vector<std::int64_t> values;
  };

  std::int64_t text(std::string_view value);
  std::uint64_t location(std::uint64_t address);
  std::uint64_t mappingOf(std::uint64_t address) const;
  static std::string keyOf(std::span<const std::uint64_t> locations, const SampleLabels &labels);

  std::vector<ValueType> sampleTypes_;
  ValueType periodType_;
  std::int64_t period_;
  std::int64_t startNanoseconds_ = 0;
  std::int64_t durationNanoseconds_ = 0;
  std::vector<std::string> comments_;
  std::string defaultSampleType_;

  std::vector<ProfileMapping> mappings_;
  std::vector<std::uint64_t> locationAddresses_;  // location id - 1 -> address
  std::unordered_map<std::uint64_t, std::uint64_t> locationIds_;
  std::vector<Sample> samples_;
  std::unordered_map<std::string, std::size_t> sampleIndex_;
  std::vector<std::string> strings_;
  std::unordered_map<std::string, std::int64_t> stringIds_;
};

}  // namespace stack_sampler
