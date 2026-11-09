#pragma once

namespace stack_sampler {

enum class ExitCode {
  // --help or --version.
  Success = 0,

  // The command line is invalid.
  UsageError = 1,
};

}  // namespace stack_sampler
