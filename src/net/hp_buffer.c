#include "hp_buffer.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

struct hp_buffer {
    unsigned char *data;
    size_t capacity;
    size_t read_pos;
    size_t write_pos;
};

static int hp_buffer_reserve(hp_buffer_t *buffer, size_t additional) {
    size_t readable = buffer->write_pos - buffer->read_pos;
    if (additional <= buffer->capacity - buffer->write_pos) return 0;
    if (buffer->read_pos > 0 && additional <= buffer->capacity - readable) {
        memmove(buffer->data, buffer->data + buffer->read_pos, readable);
        buffer->read_pos = 0;
        buffer->write_pos = readable;
        return 0;
    }
    size_t needed = readable + additional;
    size_t capacity = buffer->capacity;
    while (capacity < needed) {
        if (capacity > SIZE_MAX / 2) return -1;
        capacity *= 2;
    }
    unsigned char *new_data = (unsigned char *)realloc(buffer->data, capacity);
    if (new_data == NULL) return -1;
    if (buffer->read_pos > 0) memmove(new_data, new_data + buffer->read_pos, readable);
    buffer->data = new_data;
    buffer->capacity = capacity;
    buffer->read_pos = 0;
    buffer->write_pos = readable;
    return 0;
}

hp_buffer_t *hp_buffer_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 4096;
    hp_buffer_t *buffer = (hp_buffer_t *)calloc(1, sizeof(*buffer));
    if (buffer == NULL) return NULL;
    buffer->data = (unsigned char *)malloc(initial_capacity);
    if (buffer->data == NULL) { free(buffer); return NULL; }
    buffer->capacity = initial_capacity;
    return buffer;
}

void hp_buffer_destroy(hp_buffer_t *buffer) {
    if (buffer == NULL) return;
    free(buffer->data);
    free(buffer);
}

size_t hp_buffer_readable(const hp_buffer_t *buffer) { return buffer->write_pos - buffer->read_pos; }
const unsigned char *hp_buffer_data(const hp_buffer_t *buffer) { return buffer->data + buffer->read_pos; }

int hp_buffer_append(hp_buffer_t *buffer, const void *data, size_t length) {
    if (length == 0) return 0;
    if (hp_buffer_reserve(buffer, length) != 0) return -1;
    memcpy(buffer->data + buffer->write_pos, data, length);
    buffer->write_pos += length;
    return 0;
}

void hp_buffer_consume(hp_buffer_t *buffer, size_t length) {
    size_t readable = hp_buffer_readable(buffer);
    if (length >= readable) { buffer->read_pos = 0; buffer->write_pos = 0; }
    else buffer->read_pos += length;
}

ssize_t hp_buffer_read_fd(hp_buffer_t *buffer, int fd, int *saved_errno) {
    unsigned char extra[65536];
    struct iovec vectors[2] = {
        { buffer->data + buffer->write_pos, buffer->capacity - buffer->write_pos },
        { extra, sizeof(extra) }
    };
    ssize_t result = readv(fd, vectors, 2);
    if (result <= 0) { if (saved_errno != NULL) *saved_errno = errno; return result; }
    size_t direct = (size_t)result;
    size_t available = buffer->capacity - buffer->write_pos;
    if (direct > available) {
        direct = available;
        if (hp_buffer_append(buffer, extra, (size_t)result - direct) != 0) {
            if (saved_errno != NULL) *saved_errno = ENOMEM;
            return -1;
        }
    }
    buffer->write_pos += direct;
    return result;
}

ssize_t hp_buffer_write_fd(hp_buffer_t *buffer, int fd, int *saved_errno) {
    ssize_t result = write(fd, hp_buffer_data(buffer), hp_buffer_readable(buffer));
    if (result > 0) hp_buffer_consume(buffer, (size_t)result);
    else if (saved_errno != NULL) *saved_errno = errno;
    return result;
}
