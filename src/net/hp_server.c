#define _GNU_SOURCE
#include "hp_server.h"
#include "hp_buffer.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define HP_MAX_EVENTS 256

struct hp_connection {
    int fd;
    hp_buffer_t *input;
    hp_buffer_t *output;
    struct hp_server *server;
    int closing;
};

struct hp_server {
    int listen_fd;
    int epoll_fd;
    int wake_fd;
    int running;
    hp_server_config_t config;
    char *bind_address;
    hp_connection_t **connections;
    size_t connection_capacity;
    hp_on_connect_fn on_connect;
    hp_on_message_fn on_message;
    hp_on_close_fn on_close;
    void *user_data;
};

static int epoll_update(hp_server_t *server, int fd, uint32_t events, void *data) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = events;
    event.data.ptr = data;
    return epoll_ctl(server->epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

static void connection_close(hp_connection_t *connection) {
    hp_server_t *server = connection->server;
    if (connection->closing) return;
    connection->closing = 1;
    epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, connection->fd, NULL);
    close(connection->fd);
    if (server->on_close != NULL) server->on_close(connection, server->user_data);
    if ((size_t)connection->fd < server->connection_capacity) server->connections[connection->fd] = NULL;
    hp_buffer_destroy(connection->input);
    hp_buffer_destroy(connection->output);
    free(connection);
}

static void accept_connections(hp_server_t *server) {
    for (;;) {
        int fd = accept4(server->listen_fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            return;
        }
        if ((size_t)fd >= server->connection_capacity) { close(fd); continue; }
        hp_connection_t *connection = (hp_connection_t *)calloc(1, sizeof(*connection));
        if (connection == NULL) { close(fd); continue; }
        connection->fd = fd;
        connection->server = server;
        connection->input = hp_buffer_create(4096);
        connection->output = hp_buffer_create(4096);
        if (connection->input == NULL || connection->output == NULL) {
            hp_buffer_destroy(connection->input); hp_buffer_destroy(connection->output);
            free(connection); close(fd); continue;
        }
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
        event.data.ptr = connection;
        if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, fd, &event) != 0) {
            hp_buffer_destroy(connection->input); hp_buffer_destroy(connection->output);
            free(connection); close(fd); continue;
        }
        server->connections[fd] = connection;
        if (server->on_connect != NULL) server->on_connect(connection, server->user_data);
    }
}

static void handle_connection(hp_connection_t *connection, uint32_t events) {
    hp_server_t *server = connection->server;
    if ((events & (EPOLLERR | EPOLLHUP)) != 0) { connection_close(connection); return; }
    if ((events & EPOLLIN) != 0) {
        for (;;) {
            int saved_errno = 0;
            ssize_t count = hp_buffer_read_fd(connection->input, connection->fd, &saved_errno);
            if (count > 0) {
                if (hp_buffer_readable(connection->input) > server->config.max_read_buffer) {
                    connection_close(connection); return;
                }
                if (server->on_message != NULL) {
                    server->on_message(connection, hp_buffer_data(connection->input),
                                       hp_buffer_readable(connection->input), server->user_data);
                    if (connection->closing) return;
                    hp_buffer_consume(connection->input, hp_buffer_readable(connection->input));
                }
                if ((size_t)count < 65536U) break;
                continue;
            }
            if (count == 0) { connection_close(connection); return; }
            if (saved_errno == EINTR) continue;
            if (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK) break;
            connection_close(connection); return;
        }
    }
    if ((events & EPOLLOUT) != 0) {
        while (hp_buffer_readable(connection->output) > 0) {
            int saved_errno = 0;
            ssize_t count = hp_buffer_write_fd(connection->output, connection->fd, &saved_errno);
            if (count > 0) continue;
            if (saved_errno == EINTR) continue;
            if (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK) break;
            connection_close(connection); return;
        }
        uint32_t wanted = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
        if (hp_buffer_readable(connection->output) > 0) wanted |= EPOLLOUT;
        (void)epoll_update(server, connection->fd, wanted, connection);
    }
    if ((events & EPOLLRDHUP) != 0) connection_close(connection);
}

hp_server_t *hp_server_create(const hp_server_config_t *config) {
    if (config == NULL || config->port == 0 || config->max_read_buffer == 0) return NULL;
    hp_server_t *server = (hp_server_t *)calloc(1, sizeof(*server));
    if (server == NULL) return NULL;
    server->listen_fd = -1; server->epoll_fd = -1; server->wake_fd = -1;
    server->config = *config;
    server->config.backlog = config->backlog > 0 ? config->backlog : 128;
    server->bind_address = strdup(config->bind_address != NULL ? config->bind_address : "0.0.0.0");
    server->connection_capacity = 65536;
    server->connections = (hp_connection_t **)calloc(server->connection_capacity, sizeof(*server->connections));
    if (server->bind_address == NULL || server->connections == NULL) { hp_server_destroy(server); return NULL; }
    return server;
}

void hp_server_set_callbacks(hp_server_t *server, hp_on_connect_fn on_connect, hp_on_message_fn on_message,
                             hp_on_close_fn on_close, void *user_data) {
    if (server == NULL) return;
    server->on_connect = on_connect; server->on_message = on_message; server->on_close = on_close; server->user_data = user_data;
}

int hp_server_start(hp_server_t *server) {
    if (server == NULL) return -1;
    server->listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (server->listen_fd < 0) return -1;
    int reuse = 1;
    if (setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) return -1;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); address.sin_family = AF_INET; address.sin_port = htons(server->config.port);
    if (inet_pton(AF_INET, server->bind_address, &address.sin_addr) != 1) return -1;
    if (bind(server->listen_fd, (struct sockaddr *)&address, sizeof(address)) != 0) return -1;
    if (listen(server->listen_fd, server->config.backlog) != 0) return -1;
    server->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (server->epoll_fd < 0) return -1;
    struct epoll_event event; memset(&event, 0, sizeof(event)); event.events = EPOLLIN; event.data.ptr = server;
    if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->listen_fd, &event) != 0) return -1;
    server->running = 1;
    return 0;
}

void hp_server_run(hp_server_t *server) {
    if (server == NULL) return;
    struct epoll_event events[HP_MAX_EVENTS];
    while (server->running) {
        int count = epoll_wait(server->epoll_fd, events, HP_MAX_EVENTS, -1);
        if (count < 0) { if (errno == EINTR) continue; break; }
        for (int i = 0; i < count; ++i) {
            if (events[i].data.ptr == server) accept_connections(server);
            else handle_connection((hp_connection_t *)events[i].data.ptr, events[i].events);
        }
    }
}

void hp_server_stop(hp_server_t *server) { if (server != NULL) server->running = 0; }

int hp_connection_send(hp_connection_t *connection, const void *data, size_t length) {
    if (connection == NULL || connection->closing || hp_buffer_append(connection->output, data, length) != 0) return -1;
    uint32_t events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    return epoll_update(connection->server, connection->fd, events, connection);
}

int hp_connection_fd(const hp_connection_t *connection) { return connection == NULL ? -1 : connection->fd; }

void hp_server_destroy(hp_server_t *server) {
    if (server == NULL) return;
    server->running = 0;
    if (server->connections != NULL) {
        for (size_t i = 0; i < server->connection_capacity; ++i) if (server->connections[i] != NULL) connection_close(server->connections[i]);
    }
    if (server->listen_fd >= 0) close(server->listen_fd);
    if (server->epoll_fd >= 0) close(server->epoll_fd);
    free(server->connections); free(server->bind_address); free(server);
}
