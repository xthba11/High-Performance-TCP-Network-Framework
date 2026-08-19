# Project Scope

## Phase-1 (Must Deliver)
- Reactor + epoll based non-blocking TCP core.
- Application-level Buffer with sticky/partial packet handling.
- Timer subsystem (timing wheel preferred) for timeout/heartbeat.
- 1 main loop + N I/O loops + bounded task thread pool.
- One runnable application (HTTP server or Redis-like KV server).
- Reproducible build, test, benchmark, and troubleshooting documents.

## Explicitly Out of Scope (Current Phase)
- SSL/TLS
- HTTP/2 and HTTP/3
- Cluster mode / distributed scheduling
- Durable persistence such as AOF/RDB
- Cross-platform support beyond Linux x86_64

## Reliability Constraints
- TCP data must not be silently dropped.
- Old telemetry frames may be dropped only with explicit policy and logging.
- All performance numbers must be measured and traceable to raw logs.
