#include "helpers.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

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

// Now there is something I want to say regarding this function
// While reading in increments of one character IS bad and slow,
// it seems to me a much better idea than to do it otherwise
// as otherwise this function "returns buffer with zero to count
// delimiters in it" and this seems like bad API design, not
// to mention that there will be some leftover data we can't
// force back into file descriptor, so that we can't use any other
// reading functions on that file descriptor if we want any meaningful
// results. Therefore, I decalre this task bad, and its author
// should feel bad. Or write tasks in a more sensible way.
// Or something. This implementation also conforms to task description,
// although is slow.
// Some may propose alternate "solutions", like "ignore all of
// reasons for bad API design above". It's not a solution.
// Proper solution would be [pseudo]-object-oriented buffered
// I/O which is clearly out of scope of this task.
ssize_t read_until(int fd, void *buf, size_t count, char delimiter) {
    size_t offset = 0;
    for(;;) {
        ssize_t rr = read(fd, buf + offset, 1);
        if(rr == -1)
            return -1;
        if(rr == 0)
            return offset;
        if(((char*) buf)[offset++] == delimiter)
            return offset;
        if(offset == count)
            return count;
    }
}

int spawn(const char* file, char* const argv[]) {
    int pid = fork();
    if(pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return status;
    } else if(pid == 0) {
        exit(execvp(file, argv));
    } else {
        return -1;
    }
}