#define _POSIX_SOURCE
#define _GNU_SOURCE

#include "helpers.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

#include<stdio.h>

#ifdef NO_STRING_H
static size_t strlen(char *str) {
    char* i = str;
    while(*i) i++;
    return i - str;
}

static void* memcpy(void *dst_v, void* src_v, size_t size) {
    char *dst = (char*) dst_v;
    char *src = (char*) src_v;
    for(int i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    return dst_v;
}

static void* memset(void *dst_v, int val, size_t size) {
    char* dst = (char*) dst_v;
    while(size--) {
        dst[size] = (char) val;
    }
    return dst;
}
#else
#include<string.h>
#endif

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
        /*int nullFd = open("/dev/null", O_WRONLY);
        if(nullFd < 0)
            return -2;
        if(dup2(nullFd, STDOUT_FILENO) < 0)
            return -3;
        close(nullFd);*/ // soapy task descriptions are killing me
        exit(execvp(file, argv));
    } else {
        return -1;
    }
}

execargs_t make_execargs(char **args) {
    size_t count = 0;
    for(char** i = args; *i; count++, i++) ;
    count++;
    char ** rv = (char**) malloc(count * sizeof(void*));
    if(!rv)
        return rv;
    for(size_t i = 0; i < count; i++) {
        if(!args[i]) {
            rv[i] = args[i];
            continue;
        }
        size_t isz = strlen(args[i]);
        char* new_str = malloc(isz + 1);
        if(!new_str) {
            for(size_t j = 0; j < i; j++) {
                free(rv[i]);
            }
            free(rv);
            return 0;
        }
        memcpy(new_str, args[i], isz + 1);
        rv[i] = new_str;
    }
    return rv;
}

void free_execargs(execargs_t args_e) {
    for(char** i = args_e; *i; i++) {
        free(*i);
    }
    free(args_e);
}

int exec(execargs_t *args) {
    int run = 1;
//    while(run)
//        usleep(100000);
    return execvp(**args, *args);
}

int runpiped(execargs_t **args, size_t n) { // totally not thread-safe, because c++ is required to make it thread-safe without writing infinite amounts of code
    int pipes[2*n - 2];
    for(size_t i = 1; i < n; i++) {
        int res = pipe2(pipes + 2 * (i - 1), O_CLOEXEC);
        if(res) {
            for(int j = 1; j < i; j++) { // or I could have used C++ with RAII and other cool stuff, you know
                close(pipes[j * 2 - 2]);
                close(pipes[j * 2 - 1]);
            }
            return -1;
        }
    }
    
    pid_t pids[n];
    memset(pids, 0, n * sizeof(pid_t));
    
    sigset_t mask;
    sigset_t orig_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &orig_mask);
    
    for(size_t i = 0; i < n; i++) {
        int pid = fork();
        if(pid < 0) {
            goto out_of_while; // kill all we have remaining
        } else if(pid) {
            pids[i] = pid;
        } else {
            if(i > 0)
                dup2(pipes[i * 2 - 2], STDIN_FILENO);
            if(i < n - 1)
                dup2(pipes[i * 2 + 1], STDOUT_FILENO);
            printf("EXEC FAILD LOL %d\n\n", exec(args[i]));
            exit(-1);
        }
    }
    
    for(size_t i = 1; i < n; i++) {
        close(pipes[i * 2 - 2]);
        close(pipes[i * 2 - 1]);
    }
    
    siginfo_t info;
    int killed_procs = 0;
    while(1) {
        sigwaitinfo(&mask, &info); // wait for sigchld or sigint, they are blocked anyway
        if(info.si_signo == SIGINT)
            break;
        if(info.si_signo == SIGCHLD) {
            int chld;
            while((chld = waitpid(-1, 0, WNOHANG)) > 0) {
                for(int i = 0; i < n; i++) {
                    if(pids[i] == chld) {
                        pids[i] = 0;
                        break;
                    }
                }
                killed_procs++;
                if(killed_procs == n)
                    goto out_of_while;
            }
        }
    }
    out_of_while:
    
    for(int i = 0; i < n; i++) {
        if(pids[i]) {
            kill(pids[i], SIGKILL);
            waitpid(pids[i], 0, 0); // collect info so no zombies remain
        }
    }
    
    sigprocmask(SIG_SETMASK, &orig_mask, 0); // restore original mask
    return 1;
}
