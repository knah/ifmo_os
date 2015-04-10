#include <helpers.h>
#include <bufio.h>

#define BUFFER_SIZE 4096

char newline = '\n';

int main(int argc, char* argv[]) {
    char* path = argv[1];
    for(int i = 0; i < argc - 1; i++)
        argv[i] = argv[i + 1];

    char buf[BUFFER_SIZE];
    argv[argc - 1] = buf;

    buf_t *buffota = buf_new(BUFFER_SIZE);

    for(;;) {
        int res = buf_getline(STDIN_FILENO, buffota, '\n', buf);
        if(res == -1) {
            return 1;
        }
        if(res == 0) {
            return 0;
        }
        buf[res] = 0;
        if((res % 2) != 0) {
            continue;
        }
        int res2 = spawn(path, argv);
        if(res2 == 0) {
            if(write_(STDOUT_FILENO, buf, res) < 0)
                return 2;
            if(write_(STDOUT_FILENO, &newline, 1) < 0)
                return 3;
        }
    }
}
