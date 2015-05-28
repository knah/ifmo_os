#define _POSIX_SOURCE
#define _GNU_SOURCE

#include "helpers.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

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
    return execvp(**args, *args);
}

// Another crappy task definition rant:
// So, crappy task definitions. Didn't read it until it
// was too late to tell that it's crappy. Why is it crappy
// this time? Because of ridiculous exit conditions for runpiped.
// Wait for one process to die, then kill all of them?
// Sounds like an amazing data race generator and horrible
// software design. How is sleep 0 | (any commands) supposed to work?
// Should it fail? Sure it doesn't fail in bash. And in any other
// sensible shell too.
// But that's not all. How are we even supposed to track pipe state?
// I have no idea if pipe is closed or open by other process, and I
// have not a single clue on how to track it's state. Sure, probably
// I could inject wrapper into child processes that intercepts
// calls to close() and sends signals to parent, but... It's a bit
// too much for relatively simple homework. Additionally, bash doesn't
// do that either. Commands stop executing because THEY see eof or
// closed write end of pipe, whatever it does. And programs close
// stdin and stdout if they don't need them. That's why commands like
// find /home | grep \.c | head finish running as soon as 10 elements
// are outputted. It's because head exits, closing it's end of pipe.
// Grep exits because it has nowhere to write results.
// And find exits because of the same reason. NOT because shell detects
// closed pipe and kills them.
// -----
// TLDR: do your research before writing absurd tasks. Or better,
// complete your tasks yourself before giving it to others.
// PS: I did ltrace on bash. No pipe-watching or kills in there.
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
    
    int fork_failed = 0;
    for(size_t i = 0; i < n; i++) {
        int pid = fork();
        if(pid < 0) {
            fork_failed = 1;
            goto no_forks; // kill all we have remaining
        } else if(pid) {
            pids[i] = pid;
        } else {
            if(i > 0)
                dup2(pipes[i * 2 - 2], STDIN_FILENO);
            if(i < n - 1)
                dup2(pipes[i * 2 + 1], STDOUT_FILENO);
            exec(args[i]);
            char errname[strlen(args[i][0][0]) + 150];
            int saved_errno = errno;
            switch(errno) {
                case EACCES:
                    sprintf(errname, "Failed to run %s: access denied\n", args[i][0][0]);
                    break;
                case EIO:
                    sprintf(errname, "Failed to run %s: I/O error\n", args[i][0][0]);
                    break;
                case ENOEXEC:
                    sprintf(errname, "Failed to run %s: not executable\n", args[i][0][0]);
                    break;
                case ENOENT:
                    sprintf(errname, "Failed to run %s: file not found\n", args[i][0][0]);
                    break;
                default:
                    sprintf(errname, "Failed to run %s: errno=%d\n", args[i][0][0], saved_errno);
            }
            write_(STDERR_FILENO, errname, strlen(errname));
            exit(-1);
        }
    }
    no_forks:
    
    for(size_t i = 1; i < n; i++) {
        close(pipes[i * 2 - 2]);
        close(pipes[i * 2 - 1]);
    }
    
    if(fork_failed)
        goto out_of_while;
    
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
            kill(pids[i], SIGKILL); // just to make sure it's dead
            waitpid(pids[i], 0, 0); // collect info so no zombies remain
        }
    }
    
    sigprocmask(SIG_SETMASK, &orig_mask, 0); // restore original mask
    return -fork_failed;
}
