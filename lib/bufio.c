#include "bufio.h"
#include <string.h>
#include <stdlib.h>

#ifdef DEBUG
#define DEBUG_ASSERT(cond) if(!(cond)) abort();
#else
#define DEBUG_ASSERT(cond)
#endif

char *buf_get_data(buf_t *buf) {
    DEBUG_ASSERT(buf != 0);
    
    return ((char*) buf) + 2 * sizeof(size_t);
}

struct buf_t *buf_new(size_t capacity) {
    buf_t *rv = (buf_t*) malloc(sizeof(size_t) * 2 + capacity);
    if(rv == 0)
        return rv;
    rv->size = 0;
    rv->capacity = capacity;
    return rv;
}

void buf_free(struct buf_t *buf) {
    free(buf);
}

size_t buf_capacity(buf_t *buf) {
    DEBUG_ASSERT(buf != 0);
    return buf->capacity;
}

size_t buf_size(buf_t *buf) {
    DEBUG_ASSERT(buf != 0);
    return buf->size;
}

ssize_t buf_fill(int fd, buf_t *buf, size_t required) {
    char *data = buf_get_data(buf); // assert is inside of it
    DEBUG_ASSERT(required <= buf->capacity);
    
    while(buf->size < required) {
        ssize_t rres = read(fd, data + buf->size, buf->capacity - buf->size);
        if(rres < 0) {
            return -1; // errno was set by read()
        }
        if(rres == 0) {
            return buf->size;
        }
        buf->size += rres;
    }
    return buf->size;
}

ssize_t buf_flush(int fd, buf_t *buf, size_t required) {
    char *data = buf_get_data(buf);
    
    size_t offset = 0;
    while(buf->size > 0 && offset < required) {
        ssize_t wres = write(fd, data + offset, buf->size - offset);
        if(wres < 0) {
            memmove(data, data + offset, buf->size - offset);
            buf->size -= offset;
            return -1;
        }
        offset += wres;
    }
    memmove(data, data + offset, buf->size - offset);
    buf->size -= offset;
    return offset;
}

ssize_t buf_getline(int fd, buf_t *buf, char sep, void *ebuf) {
    ssize_t rd = buf_fill(fd, buf, buf->size + 1);
    if(rd < 0)
        return rd;
    char *data = buf_get_data(buf);
    int spos = buf->size;
    int mpos = buf->size;
    for(int i = 0; i < buf->size; i++) {
        if(data[i] == sep) {
            spos = i;
            mpos = i + 1;
            break;
        }
    }
    if(spos == 0 && buf->size > 0) {
        memmove(data, data + 1, buf->size - 1);
        buf->size--;
        return buf_getline(fd, buf, sep, ebuf);
    }
    memcpy(ebuf, data, spos);
    memmove(data, data + mpos, buf->size - mpos);
    buf->size -= mpos;
    return spos;
}
