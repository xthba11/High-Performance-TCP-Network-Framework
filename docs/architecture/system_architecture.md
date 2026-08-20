# System architecture

## Current implementation

The C library owns sockets, epoll, connections and byte buffers. It exposes opaque handles and callbacks through `include/hp_server.h`. The C++ layer owns protocol framing and application behavior.

The current event loop is single-threaded and uses level-triggered epoll. A readable connection is drained until `EAGAIN`; a writable connection is drained until its output buffer is empty or `EAGAIN` is returned.

## Ownership

- `hp_server_t` owns the listening fd, epoll fd and connection table.
- `hp_connection_t` owns its input and output buffers.
- The server callback is invoked synchronously on the event-loop thread.
- Application callbacks must not block the event loop.
- `hp_connection_send` queues bytes and enables `EPOLLOUT`.

## Protocol boundary

The C layer delivers arbitrary byte chunks. The C++ application keeps a per-connection byte vector and parses zero or more complete frames from it. This separation is intentional: TCP framing is an application concern, while socket readiness is a transport concern.

## Known limitations

The current MVP does not yet include the planned IO thread pool, eventfd task queue, timerfd idle timeout, bounded output watermarks, metrics, TLS, authentication or fuzzing. These are subsequent increments, not silently claimed features.
