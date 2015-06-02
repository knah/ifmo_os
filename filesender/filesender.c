#define _POSIX_C_SOURCE 201505

#include <bufio.h>
#include <helpers.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE 4096
#define LISTEN_QUEUE 5

int main(int argc, char** argv) {
	if(argc < 3) {
		write_(STDOUT_FILENO, "Usage: <port> <file>\n", 21);
		return 10;
	}
	signal(SIGCHLD, SIG_IGN); // auto-collect all zombies (allowed since POSIX.1-2001)
	struct addrinfo *localhost;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	if(getaddrinfo("localhost", argv[1], 0, &localhost)) {
		return 1;
	}
	int sock = socket(localhost->ai_family, SOCK_STREAM, 0);
	if(sock < 0) {
		return 2;
	}
	if(bind(sock, localhost->ai_addr, localhost->ai_addrlen)) {
		return 3;
	}
	if(listen(sock, LISTEN_QUEUE)) {
		return 5;
	}
	freeaddrinfo(localhost);
	while(1) {
		int cli = accept(sock, 0, 0);
		if(cli < 0) {
			return 4;
		}
		pid_t pid = fork();
		if(pid < 0) {
			return 5;
		}
		if(pid) {
			close(cli);
			continue;
		} else {
			close(sock);
			int file = open(argv[2], O_RDONLY);
			if(file < 0) {
				return 1;
			}
			buf_t *buf = buf_new(BUFFER_SIZE);
			if(!buf) {
				return 2;
			}
			while(1) {
				int bfr = buf_fill(file, buf, 1);
				if(bfr < 0) {
					if(buf_flush(cli, buf, buf_size(buf)) < 0)
						return 3;
					return 4;
				}
				if(bfr == 0)
					return 0;
				if(buf_flush(cli, buf, buf_size(buf)) < 0)
					return 5;
			}
		}
	}
	return 0;
}
