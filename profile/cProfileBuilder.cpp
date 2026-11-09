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

#include "cProfileBuilder.hpp"

#include <algorithm>

#include "cProtobufEncoder.hpp"

namespace stack_sampler {

namespace {

// Field numbers of profile.proto (github.com/google/pprof, proto/profile.proto).
namespace profile {
constexpr int kSampleType = 1, kSample = 2, kMapping = 3, kLocation = 4, kStringTable = 6, kTimeNanos = 9,
              kDurationNanos = 10, kPeriodType = 11, kPeriod = 12, kComment = 13, kDefaultSampleType = 14;
}
namespace value_type {
constexpr int kType = 1, kUnit = 2;
}
namespace sample {
constexpr int kLocationId = 1, kValue = 2, kLabel = 3;
}
namespace label {
constexpr int kKey = 1, kStr = 2, kNum = 3;
}
namespace mapping {
constexpr int kId = 1, kMemoryStart = 2, kMemoryLimit = 3, kFileOffset = 4, kFilename = 5, kBuildId = 6;
}
namespace location {
constexpr int kId = 1, kMappingId = 2, kAddress = 3;
}

}  // namespace

ProfileBuilder::ProfileBuilder(std::vector<ValueType> sampleTypes, ValueType periodType, std::int64_t period)
    : sampleTypes_(std::move(sampleTypes)), periodType_(std::move(periodType)), period_(period) {
  text("");  // string_table[0] must be empty
}

void ProfileBuilder::addMapping(const ProfileMapping &mapping) {
  const auto same = [&](const ProfileMapping &known) {
    return known.start == mapping.start && known.limit == mapping.limit && known.fileOffset == mapping.fileOffset &&
           known.filename == mapping.filename;
  };
  if (std::none_of(mappings_.begin(), mappings_.end(), same)) mappings_.push_back(mapping);
}

void ProfileBuilder::addSample(std::span<const std::uint64_t> addresses, const SampleLabels &labels,
                               std::span<const std::int64_t> values) {
  std::vector<std::uint64_t> locations;
  locations.reserve(addresses.size());
  for (std::uint64_t address : addresses) locations.push_back(location(address));

  const std::string key = keyOf(locations, labels);
  const auto found = sampleIndex_.find(key);
  if (found != sampleIndex_.end()) {
    Sample &known = samples_[found->second];
    for (std::size_t i = 0; i < values.size() && i < known.values.size(); ++i) known.values[i] += values[i];
    return;
  }
  sampleIndex_.emplace(key, samples_.size());
  samples_.push_back(Sample{std::move(locations), labels, std::vector<std::int64_t>(values.begin(), values.end())});
}

void ProfileBuilder::setTime(std::int64_t startNanoseconds, std::int64_t durationNanoseconds) {
  startNanoseconds_ = startNanoseconds;
  durationNanoseconds_ = durationNanoseconds;
}

void ProfileBuilder::addComment(std::string comment) { comments_.push_back(std::move(comment)); }

std::string ProfileBuilder::serialize() {
  ProtobufEncoder out;
  for (const ValueType &type : sampleTypes_) {
    ProtobufEncoder value;
    value.signedVarint(value_type::kType, text(type.first));
    value.signedVarint(value_type::kUnit, text(type.second));
    out.message(profile::kSampleType, value);
  }
  for (const Sample &one : samples_) {
    ProtobufEncoder encoded;
    encoded.packedVarints(sample::kLocationId, one.locations);
    std::vector<std::uint64_t> values(one.values.begin(), one.values.end());
    encoded.packedVarints(sample::kValue, values);
    for (const auto &[key, value] : one.labels.strings) {
      ProtobufEncoder pair;
      pair.signedVarint(label::kKey, text(key));
      pair.signedVarint(label::kStr, text(value));
      encoded.message(sample::kLabel, pair);
    }
    for (const auto &[key, value] : one.labels.numbers) {
      ProtobufEncoder pair;
      pair.signedVarint(label::kKey, text(key));
      pair.signedVarint(label::kNum, value);
      encoded.message(sample::kLabel, pair);
    }
    out.message(profile::kSample, encoded);
  }
  for (std::size_t i = 0; i < mappings_.size(); ++i) {
    const ProfileMapping &known = mappings_[i];
    ProtobufEncoder encoded;
    encoded.varint(mapping::kId, i + 1);
    encoded.varint(mapping::kMemoryStart, known.start);
    encoded.varint(mapping::kMemoryLimit, known.limit);
    encoded.varint(mapping::kFileOffset, known.fileOffset);
    encoded.signedVarint(mapping::kFilename, text(known.filename));
    if (!known.buildId.empty()) encoded.signedVarint(mapping::kBuildId, text(known.buildId));
    out.message(profile::kMapping, encoded);
  }
  for (std::size_t i = 0; i < locationAddresses_.size(); ++i) {
    ProtobufEncoder encoded;
    encoded.varint(location::kId, i + 1);
    const std::uint64_t mappingId = mappingOf(locationAddresses_[i]);
    if (mappingId != 0) encoded.varint(location::kMappingId, mappingId);
    encoded.varint(location::kAddress, locationAddresses_[i]);
    out.message(profile::kLocation, encoded);
  }
  out.signedVarint(profile::kTimeNanos, startNanoseconds_);
  out.signedVarint(profile::kDurationNanos, durationNanoseconds_);
  {
    ProtobufEncoder value;
    value.signedVarint(value_type::kType, text(periodType_.first));
    value.signedVarint(value_type::kUnit, text(periodType_.second));
    out.message(profile::kPeriodType, value);
  }
  out.signedVarint(profile::kPeriod, period_);
  for (const std::string &comment : comments_) out.signedVarint(profile::kComment, text(comment));
  if (!defaultSampleType_.empty()) out.signedVarint(profile::kDefaultSampleType, text(defaultSampleType_));
  // The string table goes last: every index above is already in it.
  for (const std::string &value : strings_) out.bytes(profile::kStringTable, value);
  return out.data();
}

std::int64_t ProfileBuilder::text(std::string_view value) {
  const std::string key(value);
  const auto found = stringIds_.find(key);
  if (found != stringIds_.end()) return found->second;
  const auto id = static_cast<std::int64_t>(strings_.size());
  strings_.push_back(key);
  stringIds_.emplace(key, id);
  return id;
}

std::uint64_t ProfileBuilder::location(std::uint64_t address) {
  const auto found = locationIds_.find(address);
  if (found != locationIds_.end()) return found->second;
  locationAddresses_.push_back(address);
  const std::uint64_t id = locationAddresses_.size();
  locationIds_.emplace(address, id);
  return id;
}

std::uint64_t ProfileBuilder::mappingOf(std::uint64_t address) const {
  for (std::size_t i = 0; i < mappings_.size(); ++i)
    if (address >= mappings_[i].start && address < mappings_[i].limit) return i + 1;
  return 0;
}

std::string ProfileBuilder::keyOf(std::span<const std::uint64_t> locations, const SampleLabels &labels) {
  std::string key;
  for (std::uint64_t id : locations) key.append(std::to_string(id)).push_back(',');
  key.push_back('|');
  for (const auto &[name, value] : labels.strings) key.append(name).append("=").append(value).push_back('\n');
  for (const auto &[name, value] : labels.numbers)
    key.append(name).append("=").append(std::to_string(value)).push_back('\n');
  return key;
}

}  // namespace stack_sampler
