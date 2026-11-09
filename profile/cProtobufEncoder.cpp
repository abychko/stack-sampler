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

#include "cProtobufEncoder.hpp"

namespace stack_sampler {

namespace {

constexpr int kVarint = 0;
constexpr int kLengthDelimited = 2;

}  // namespace

void ProtobufEncoder::varint(int field, std::uint64_t value) {
  tag(field, kVarint);
  rawVarint(value);
}

void ProtobufEncoder::bytes(int field, std::string_view value) {
  tag(field, kLengthDelimited);
  rawVarint(value.size());
  data_.append(value);
}

void ProtobufEncoder::packedVarints(int field, std::span<const std::uint64_t> values) {
  if (values.empty()) return;
  ProtobufEncoder packed;
  for (std::uint64_t value : values) packed.rawVarint(value);
  bytes(field, packed.data_);
}

void ProtobufEncoder::tag(int field, int wireType) {
  rawVarint((static_cast<std::uint64_t>(field) << 3) | static_cast<std::uint64_t>(wireType));
}

void ProtobufEncoder::rawVarint(std::uint64_t value) {
  while (value >= 0x80) {
    data_.push_back(static_cast<char>((value & 0x7F) | 0x80));
    value >>= 7;
  }
  data_.push_back(static_cast<char>(value));
}

}  // namespace stack_sampler
