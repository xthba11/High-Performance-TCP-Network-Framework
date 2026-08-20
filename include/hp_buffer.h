#ifndef HP_BUFFER_H
#define HP_BUFFER_H

#include <stddef.h>
#include <sys/types.h>

typedef struct hp_buffer hp_buffer_t;

hp_buffer_t *hp_buffer_create(size_t initial_capacity);
void hp_buffer_destroy(hp_buffer_t *buffer);
size_t hp_buffer_readable(const hp_buffer_t *buffer);
const unsigned char *hp_buffer_data(const hp_buffer_t *buffer);
int hp_buffer_append(hp_buffer_t *buffer, const void *data, size_t length);
void hp_buffer_consume(hp_buffer_t *buffer, size_t length);
ssize_t hp_buffer_read_fd(hp_buffer_t *buffer, int fd, int *saved_errno);
ssize_t hp_buffer_write_fd(hp_buffer_t *buffer, int fd, int *saved_errno);

#endif
