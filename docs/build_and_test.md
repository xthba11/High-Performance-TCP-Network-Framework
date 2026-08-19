# Build and Test

## Prerequisites
- Linux x86_64
- CMake >= 3.16
- GCC >= 9 (or compatible Clang)

## Build
```bash
bash scripts/build_debug.sh
bash scripts/build_release.sh
```

## Run
```bash
./build-debug/bin/hello
./build-release/bin/hello
```

## Test
```bash
bash scripts/run_tests.sh
```

## Optional Sanitizers
Pass options to CMake during configure:
- `-DHP_TCP_ENABLE_ASAN=ON`
- `-DHP_TCP_ENABLE_TSAN=ON`
- `-DHP_TCP_ENABLE_UBSAN=ON`
