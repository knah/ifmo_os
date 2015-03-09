#include "helpers.h"
#include <unistd.h>

ssize_t read_(int fd, void* buf, size_t count) {
    size_t offset = 0;
    for(;;) {
        ssize_t rr = read(fd, buf + offset, count - offset);
        if(rr == -1)
            return -1;
        offset += rr;
        if(rr == 0)
            return offset;
    }
}

ssize_t write_(int fd, const void* buf, size_t count) {
    size_t offset = 0;
    for(;;) {
        ssize_t rr = write(fd, buf + offset, count - offset);
        if(rr == -1)
            return -1;
        offset += rr;
        if(offset >= count)
            return offset;
    }
}