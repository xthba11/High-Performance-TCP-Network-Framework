# Environment Baseline

## 1) System / Kernel / Architecture
- Kernel: `Linux 6.17.0-1022-azure`
- OS: `Ubuntu 24.04.4 LTS`
- Architecture: `x86_64`

## 2) Toolchain
- GCC: `13.3.0`
- CMake: `3.31.6`

## 3) Hardware Snapshot
- CPU: `4 logical CPUs`
- Memory: `15 GiB total`

## 4) ulimit
- `ulimit -n`: `65536` (meets recommended >= 65535 for high-concurrency baseline)

## 5) Benchmark / Profiling Tools
- `wrk`: not installed
- `ab`: `/usr/bin/ab`
- `redis-benchmark`: not installed
- `perf`: `/usr/bin/perf`
- `valgrind`: not installed

## 6) Confirmed / Risks / Missing
### Confirmed
- Linux x86_64 runtime available.
- GCC and CMake versions exceed project minimum.
- Open file limit baseline is adequate for early stress tests.

### Risks / Missing
- `wrk` missing (HTTP benchmark tooling gap).
- `redis-benchmark` missing (KV benchmark tooling gap).
- `valgrind` missing (memory diagnostics gap).

## 7) Next Validation Commands
- `which wrk || echo "wrk not found"`
- `which redis-benchmark || echo "redis-benchmark not found"`
- `which valgrind || echo "valgrind not found"`
- `perf --version`
