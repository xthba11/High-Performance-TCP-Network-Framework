#ifndef HP_SERVER_H
#define HP_SERVER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hp_server hp_server_t;
typedef struct hp_connection hp_connection_t;

typedef struct hp_server_config {
    const char *bind_address;
    uint16_t port;
    int backlog;
    size_t max_read_buffer;
} hp_server_config_t;

typedef void (*hp_on_connect_fn)(hp_connection_t *, void *);
typedef void (*hp_on_message_fn)(hp_connection_t *, const void *, size_t, void *);
typedef void (*hp_on_close_fn)(hp_connection_t *, void *);

hp_server_t *hp_server_create(const hp_server_config_t *config);
void hp_server_set_callbacks(hp_server_t *server,
                             hp_on_connect_fn on_connect,
                             hp_on_message_fn on_message,
                             hp_on_close_fn on_close,
                             void *user_data);
int hp_server_start(hp_server_t *server);
void hp_server_run(hp_server_t *server);
void hp_server_stop(hp_server_t *server);
void hp_server_destroy(hp_server_t *server);
int hp_connection_send(hp_connection_t *connection, const void *data, size_t length);
int hp_connection_fd(const hp_connection_t *connection);

#ifdef __cplusplus
}
#endif

#endif
