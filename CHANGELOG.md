# Changelog

Changes by minor version; each entry includes its patch releases.

## 0.2 — 2026-11-09

### Added

- Wait profile of a running Linux process, taken without stopping it: `--pid`, `--output`, `--frequency`, `--duration`. Output is a gzip-compressed pprof profile with raw addresses, build ids and labels (thread, tid, state, wait, syscall, futex, fd).
- Exit codes 2 (the process cannot be sampled) and 3 (the profile cannot be written).

## 0.1 — 2026-11-09

### Added

- `stack-sampler --version` and `--help`.
