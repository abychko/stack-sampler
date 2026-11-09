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

namespace stack_sampler {

// Writes protobuf wire format: varint and length-delimited fields only, which
// is all profile.proto uses.
class ProtobufEncoder {
 public:
  void varint(int field, std::uint64_t value);
  void signedVarint(int field, std::int64_t value) { varint(field, static_cast<std::uint64_t>(value)); }
  void bytes(int field, std::string_view value);
  void message(int field, const ProtobufEncoder &nested) { bytes(field, nested.data()); }
  void packedVarints(int field, std::span<const std::uint64_t> values);

  const std::string &data() const { return data_; }

 private:
  void tag(int field, int wireType);
  void rawVarint(std::uint64_t value);

  std::string data_;
};

}  // namespace stack_sampler
