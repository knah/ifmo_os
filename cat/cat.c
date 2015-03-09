#include <helpers.h>
#include <stdio.h>

#define BUF_SIZE 4096

int main() {
	char buf[BUF_SIZE];
	while(1) {
		ssize_t count = read_(STDIN_FILENO, buf, BUF_SIZE);
		if(count == -1)
			return 1;
		if(count == 0)
			return 0;
		ssize_t write_rs = write_(STDOUT_FILENO, buf, count);
		if(write_rs == -1)
			return 2;
	}
	return 0;
}