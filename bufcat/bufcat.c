#include <unistd.h>
#include <bufio.h>

int main() {
	buf_t *buf = buf_new(4096);
	while(1) {
		int bfr = buf_fill(STDIN_FILENO, buf, buf_capacity(buf));
		if(bfr < 0) {
			if(buf_flush(STDOUT_FILENO, buf, buf_size(buf)) < 0)
				return 1;
			return 2;
		}
		if(bfr == 0)
			break;
		if(buf_flush(STDOUT_FILENO, buf, buf_size(buf)) < 0)
			return 3;
	}
	return 0;
}
