#include <helpers.h>

#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {
    char* path = argv[1];
    for(int i = 0; i < argc - 1; i++)
        argv[i] = argv[i + 1];

    char buf[BUFFER_SIZE];
    argv[argc - 1] = buf;

    for(;;) {
        int res = read_until(STDIN_FILENO, buf, BUFFER_SIZE - 1, '\n');
        if(res == -1) {
            return -1;
        }
        if(res == 0) {
            return 0;
        }
        int hadNL = (buf[res - 1] == '\n');
        buf[res - hadNL] = 0;
        int res2 = spawn(path, argv);
        if(res2 == 0) {
            if(hadNL)
                buf[res - hadNL] = '\n';
            write_(STDOUT_FILENO, buf, res);
        }
    }
}