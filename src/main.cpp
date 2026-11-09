#include <cstdio>
#include <string_view>

#include "eExitCode.hpp"

namespace {

using stack_sampler::ExitCode;

void printUsage(std::FILE *stream) {
  std::fprintf(stream, "Usage: %s [--version] [--help]\n", STACK_SAMPLER_NAME);
}

int exitWith(ExitCode code) { return static_cast<int>(code); }

}  // namespace

int main(int argc, char **argv) {
  if (argc == 2) {
    const std::string_view argument(argv[1]);
    if (argument == "--version") {
      std::printf("%s %s\n", STACK_SAMPLER_NAME, STACK_SAMPLER_VERSION);
      return exitWith(ExitCode::Success);
    }
    if (argument == "--help") {
      printUsage(stdout);
      return exitWith(ExitCode::Success);
    }
  }
  printUsage(stderr);
  return exitWith(ExitCode::UsageError);
}
