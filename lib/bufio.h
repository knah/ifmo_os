#ifndef BUFIO_H
#define BUFIO_H

#include <sys/types.h>

typedef struct buf_t {
    size_t size;
    size_t capacity;
} buf_t;

buf_t *buf_new(size_t capacity);
void buf_free(buf_t *);
size_t buf_capacity(buf_t *);
size_t buf_size(buf_t *);

ssize_t buf_fill(int fd, buf_t *buf, size_t required);
ssize_t buf_flush(int fd, buf_t *buf, size_t required);

ssize_t buf_getline(int fd, buf_t *buf, char linesep, void *ebuf);

#endif