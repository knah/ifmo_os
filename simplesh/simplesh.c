#include <helpers.h>
#include <bufio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define BUFFER_SIZE 4096

char newline = '\n';
char pipe_char = '|';
char space = ' ';

void print_newline(int);

void print_newline(int sig) {
    write_(STDOUT_FILENO, "\n", 1);
    signal(SIGINT, print_newline);
}

int main(int argc, char* argv[]) {
    char buf[BUFFER_SIZE];

    buf_t *buffota = buf_new(BUFFER_SIZE);
    signal(SIGINT, print_newline);

    for(;;) {
        write_(STDOUT_FILENO, "$", 1);
        ssize_t res = buf_getline(STDIN_FILENO, buffota, '\n', buf);
        if(res == -1) {
            if(errno == EINTR)
                continue;
            return 1;
        }
        if(res == 0) {
            return 0;
        }
        buf[res] = 0;
        int programs = 1;
        for(ssize_t i = 0; i < res; i++) {
            if(buf[i] == pipe_char) {
                programs++;
                buf[i] = 0;
            }
        }
        execargs_t eargs[programs];
        execargs_t *eptrs[programs];
        ssize_t last_pos = 0;
        int earg_counter = 0;
        for(ssize_t i = 0; i <= res; i++) {
            if(buf[i] == 0) {
                int arguments = (buf[i - 1] != space);
                ssize_t last_space = last_pos - 1;
                for(ssize_t j = last_pos; j < i; j++) {
                    if(buf[j] == space) {
                        if(last_space != j - 1) {
                            arguments++;
                        }
                        buf[j] = 0;
                        last_space = j;
                    }
                }
                char *proc_args[arguments + 1];
                proc_args[arguments] = 0;
                ssize_t arg_last_pos = last_pos;
                int arg_counter = 0;
                for(ssize_t j = last_pos; j <= i; j++) {
                    if(buf[j] == 0) {
                        if(arg_last_pos != j) {
                            proc_args[arg_counter++] = buf + arg_last_pos;
                        }
                        arg_last_pos = j + 1;
                    }
                }
                if(!(eargs[earg_counter++] = make_execargs(proc_args))) {
                    for(int j = 0; j < earg_counter - 1; j++) {
                        free_execargs(eargs[j]);
                    }
                    write_(STDOUT_FILENO, "Out of memory\n", 14);
                    goto loop_begin; // at this point I want to be coding in C++
                } else {
                    eptrs[earg_counter - 1] = eargs + earg_counter - 1;
                }
                last_pos = i + 1;
            }
        }
        
        runpiped(eptrs, programs);
        
        for(int i = 0; i < programs; i++) {
            free_execargs(eargs[i]);
        }
        loop_begin:
        ;
    }
}
