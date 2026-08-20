# Build and test

This first implementation targets Linux x86_64 because the core uses POSIX `epoll`, `accept4`, `eventfd`-compatible APIs and non-blocking sockets.

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
ctest --test-dir build-debug --output-on-failure
./build-debug/hp_server 9000
# in another terminal
./build-debug/hp_client 127.0.0.1 9000
```

Expected client output contains `body=hello from hp client`.

Sanitizer build:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DHP_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

The current MVP is single-threaded and intentionally uses level-triggered epoll. Thread pools, timerfd-based timeouts, bounded task queues, metrics and backpressure are follow-up increments.
