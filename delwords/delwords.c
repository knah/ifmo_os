#include <helpers.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 1275

int main(int argc, char* argv[]) {
	if(argc < 2) {
		write_(STDERR_FILENO, "Usage: delwords <string>\n", 25);
		return 3;
	}
	char buf[BUF_SIZE];
	int argsize = strlen(argv[1]);
	if(argsize >= BUF_SIZE) {
		write_(STDERR_FILENO, "Argument is too long\n", 21);
		return 4;
	}
	int offset = 0;
	while(1) {
		ssize_t count = read_(STDIN_FILENO, buf + offset, BUF_SIZE - offset);
		if(count == -1)
			return 1;
		if(count == 0) {
			if(write_(STDOUT_FILENO, buf, offset) < 0)
				return 2;
			return 0;
		}
		
		count += offset;
		
		if(count < argsize) {
			offset = count;
			continue;
		}
		
		int lastpos = 0;
		for(int i = 0; i < count - argsize + 1; i++) {
			int match = 1;
			for(int j = 0; j < argsize; j++) {
				if(argv[1][j] != buf[i + j]) {
					match = 0;
					break;
				}
			}
			if(match) {
				if(write_(STDOUT_FILENO, buf + lastpos, i - lastpos) < 0)
					return 2;
				lastpos = i = i + argsize - 1;
				lastpos++;
			}
		}
		if(lastpos < count - argsize) {
			if(write_(STDOUT_FILENO, buf + lastpos, count - argsize - lastpos) < 0)
				return 2;
			lastpos = count - argsize;
		}
		
		memmove(buf, buf + lastpos, count - lastpos);
		offset = count - lastpos;
	}
	return 0;
}