# Project Instructions

## Project
This repository implements a high-performance TCP network framework
based on Reactor pattern, epoll, non-blocking IO, and application-level
buffering. The target is Linux x86_64 with C++17 and CMake.

## Core Rules
1. Read the relevant design document before modifying code.
2. Work on only the requested module.
3. Do not make unrelated refactors.
4. Before editing, list the files you expect to change.
5. After editing, list every changed file and explain why.
6. C++ code uses C++17 and CMake.
7. No raw pointers for ownership; use `std::unique_ptr` / `std::shared_ptr`.
8. No bare `new`/`delete`; use RAII for all resources (`fd`, mutex, epoll).
9. All sockets must be non-blocking.
10. epoll must use `EPOLLET` (edge-triggered) where applicable.
11. Buffer reads must handle `EAGAIN` / `EWOULDBLOCK` correctly.
12. Each thread must support stop, wake-up and join.
13. Each resource must have a deterministic release path.
14. TCP connections must handle half-close (`EPOLLRDHUP`).
15. Timer callbacks must not block the event loop.
16. Logging must not allocate memory in signal handlers.
17. `main.cpp` only assembles modules and controls lifecycle.
18. Every failure path must produce a useful log message and error code.
19. Never fabricate benchmark results or performance numbers.
20. All test commands must be reproducible.

## Required Validation
For every code task, provide:
- build command;
- unit or minimal test command;
- expected output;
- one abnormal-path test;
- rollback instructions when system files are changed.

## Code Review Rules
Focus on:
- incorrect ownership or lifetime (fd, Buffer, TcpConnection);
- data races and deadlocks (cross-thread shared state);
- blocking operations in EventLoop (sleep, large memcpy, sync IO);
- unbounded queues or missing backpressure;
- missing EPOLLRDHUP / EPOLLHUP handling;
- Buffer overflow or incorrect watermark logic;
- timer drift or missed expirations;
- thread pool task starvation;
- missing protocol length validation;
- silent packet loss or connection drops;
- fd leak (not closed on error paths);
- memory copies in hot paths (zero-copy where possible).
