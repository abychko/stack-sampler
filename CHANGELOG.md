# Changelog

Changes by minor version; each entry includes its patch releases.

## 0.4 — 2026-11-12

### Added

- CPU profile: `--profile cpu`, or `--profile wait,cpu` for both in one file; `--cpu-frequency` sets the sampling rate, 99 Hz by default. Threads are not stopped: the kernel samples every CPU and walks user stacks along frame pointers.

### Changed

- Every profile has the sample types samples, wall and cpu and a `profile` label (wait or cpu).
- A wait pass costs half as much CPU: /proc files of threads stay open, stacks are copied as far as the chain goes, the memory map is reread every 10 s.
- Wait passes read all threads with one BPF task iterator when the kernel has BTF: about 40% less CPU than /proc, which stays as the fallback.
- `--frequency` is 10 by default instead of 1.

### Fixed

- Shares of running and waiting time of busy threads in the wait profile were wrong: passes kept a fixed phase and stayed in step with whatever the process does once a second. Each pass now runs at a random moment of its period.
- A thread that was being woken was recorded as waiting, with state `?`; it now counts as running, as in /proc.
- Running threads were missing from pprof reports of the wait profile; they now have the frame `[running]`.

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
