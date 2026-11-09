# Changelog

Changes by minor version; each entry includes its patch releases.

## 0.4 — 2026-11-09

### Added

- CPU profile: `--profile cpu`, or `--profile wait,cpu` for both in one file; `--cpu-frequency` sets the sampling rate, 99 Hz by default. Threads are not stopped: the kernel samples every CPU and walks user stacks along frame pointers.

### Changed

- Every profile has the sample types samples, wall and cpu and a `profile` label (wait or cpu).

## 0.3 — 2026-11-09

### Added

- Debian/Ubuntu package.

## 0.2 — 2026-11-09

### Added

- Wait profile of a running Linux process, taken without stopping it: `--pid`, `--output`, `--frequency`, `--duration`. Output is a gzip-compressed pprof profile with raw addresses, build ids and labels (thread, tid, state, wait, syscall, futex, fd).
- Exit codes 2 (the process cannot be sampled) and 3 (the profile cannot be written).

## 0.1 — 2026-11-09

### Added

- `stack-sampler --version` and `--help`.
