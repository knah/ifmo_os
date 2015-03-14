#include <helpers.h>
#include <stdio.h>

#define BUF_SIZE 4096 + 5

int main() {
	char buf[BUF_SIZE];
	while(1) {
		ssize_t count = read_until(STDIN_FILENO, buf, BUF_SIZE, ' ');
		if(count == -1)
			return 1;
		if(count == 0)
			return 0;
		int hitDelim = (buf[count - 1] == ' ');
		for(size_t i = 0; i < (count - hitDelim) / 2; i++) {
			char tmp = buf[i];
			buf[i] = buf[count - i - 1 - hitDelim];
			buf[count - i - 1 - hitDelim] = tmp;
		}
		ssize_t write_rs = write_(STDOUT_FILENO, buf, count);
		if(write_rs == -1)
			return 2;
	}
	return 0;
}