# System Architecture

## Runtime Data Flow
1. `ServerSocket` bind/listen on non-blocking fd.
2. `Acceptor` accepts new clients and forwards to `TcpServer`.
3. `EventLoop` waits events via `epoll`.
4. `Channel` dispatches read/write/error/hup callbacks.
5. `TcpConnection` drives connection lifecycle and buffering.
6. `Buffer` handles non-blocking I/O, sticky packets, partial packets.
7. Protocol layer parses HTTP/RESP and builds response.
8. Business handler executes route/command.
9. Timer module handles timeout, heartbeat, and delayed tasks.
10. `EventLoopThreadPool` distributes connections by loop-per-thread.
11. `TaskThreadPool` handles CPU-heavy work off I/O loops.
12. Graceful shutdown drains active connections before process exit.

## Threading Model
- Main thread: listen socket + accept + coordination.
- I/O threads: one `EventLoop` each, owning affiliated fds.
- Worker threads: bounded queue for asynchronous business tasks.

## Core Non-Functional Rules
- Non-blocking sockets only.
- epoll edge-triggered mode where applicable.
- Deterministic cleanup with RAII.
- No fabricated benchmark output.
