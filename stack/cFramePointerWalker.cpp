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

#include "cFramePointerWalker.hpp"

#include <array>

namespace stack_sampler {

namespace {

constexpr std::size_t kMaximumDepth = 256;

// A chain that ends at a zero saved frame pointer is complete; one cut by the
// end of the copy is accepted when it is long enough not to be chance.
constexpr std::size_t kMinimumOpenChain = 3;

constexpr std::uint64_t kWord = 8;

}  // namespace

StackTrace FramePointerWalker::walk(const StackCopy &stack, std::uint64_t stackPointer,
                                    std::uint64_t programCounter) const {
  StackTrace trace;
  trace.frames.push_back(programCounter);

  const std::uint64_t first = (stackPointer + kWord - 1) & ~(kWord - 1);
  for (std::uint64_t frame = first; frame + 2 * kWord <= stack.end(); frame += kWord) {
    Chain chain = follow(stack, frame);
    if (!accepted(chain)) continue;

    trace.chainFound = true;
    std::uint64_t atStackPointer = 0;
    if (frame > stackPointer && stack.readWord(stackPointer, atStackPointer) && code_.isCode(atStackPointer) &&
        followsCall(atStackPointer)) {
      trace.frames.push_back(atStackPointer - 1);
      trace.leafCallerAdded = true;
    }
    for (std::uint64_t returnAddress : chain.returnAddresses) trace.frames.push_back(returnAddress - 1);
    break;
  }
  return trace;
}

FramePointerWalker::Chain FramePointerWalker::follow(const StackCopy &stack, std::uint64_t frame) const {
  Chain chain;
  while (chain.returnAddresses.size() < kMaximumDepth) {
    std::uint64_t savedFrame = 0;
    std::uint64_t returnAddress = 0;
    if (!stack.readWord(frame, savedFrame) || !stack.readWord(frame + kWord, returnAddress)) {
      chain.end = ChainEnd::OutOfCopy;
      return chain;
    }
    if (!code_.isCode(returnAddress)) {
      chain.end = ChainEnd::Broken;
      return chain;
    }
    chain.returnAddresses.push_back(returnAddress);
    if (savedFrame == 0) {
      chain.end = ChainEnd::Zero;
      return chain;
    }
    if (savedFrame <= frame || (savedFrame & (kWord - 1)) != 0) {
      chain.end = ChainEnd::Broken;
      return chain;
    }
    frame = savedFrame;
  }
  chain.end = ChainEnd::Broken;
  return chain;
}

bool FramePointerWalker::accepted(const Chain &chain) {
  if (chain.end == ChainEnd::Zero) return !chain.returnAddresses.empty();
  return chain.returnAddresses.size() >= kMinimumOpenChain;
}

// The bytes before a return address end with an x86-64 call: E8 rel32, or one of
// the FF /2 encodings by length (register, memory, SIB, displacements).
bool FramePointerWalker::followsCall(std::uint64_t returnAddress) const {
  std::array<std::uint8_t, 7> b{};
  if (returnAddress < b.size() || !code_.readCode(returnAddress - b.size(), b)) return false;
  const auto modrm = [](std::uint8_t byte, std::uint8_t mod) {
    return (byte >> 6) == mod && ((byte >> 3) & 7) == 2;
  };
  if (b[2] == 0xE8) return true;
  if (b[5] == 0xFF && modrm(b[6], 3)) return true;
  if (b[5] == 0xFF && modrm(b[6], 0) && (b[6] & 7) != 4 && (b[6] & 7) != 5) return true;
  if (b[4] == 0xFF && modrm(b[5], 1) && (b[5] & 7) != 4) return true;
  if (b[4] == 0xFF && modrm(b[5], 0) && (b[5] & 7) == 4) return true;
  if (b[3] == 0xFF && modrm(b[4], 1) && (b[4] & 7) == 4) return true;
  if (b[1] == 0xFF && (modrm(b[2], 2) || (modrm(b[2], 0) && (b[2] & 7) == 5))) return true;
  if (b[0] == 0xFF && modrm(b[1], 2) && (b[1] & 7) == 4) return true;
  return false;
}

}  // namespace stack_sampler
