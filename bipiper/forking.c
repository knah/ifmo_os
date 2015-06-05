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

void pump_main(int cli1, int cli2) {
	buf_t *buf = buf_new(BUFFER_SIZE);
	if(!buf) {
		exit(2);
	}
	while(1) {
		int bfr = buf_fill(cli1, buf, 1);
		if(bfr < 0) {
			if(buf_flush(cli2, buf, buf_size(buf)) < 0)
				exit(3);
			exit(4);
		}
		if(bfr == 0)
			return;
		if(buf_flush(cli2, buf, buf_size(buf)) < 0)
			exit(5);
	}
}

int make_server(struct addrinfo *localhost) {
	int sock1 = socket(localhost->ai_family, SOCK_STREAM, 0);
	if(sock1 < 0) {
		return -1;
	}
	if(bind(sock1, localhost->ai_addr, localhost->ai_addrlen)) {
		return -2;
	}
	if(listen(sock1, LISTEN_QUEUE)) {
		return -3;
	}
	return sock1;
}

int main(int argc, char** argv) {
	if(argc < 3) {
		write_(STDOUT_FILENO, "Usage: <port1> <port2>\n", 23);
		return 1;
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO); // totally don't need these three
	signal(SIGCHLD, SIG_IGN); // auto-collect all zombies (allowed since POSIX.1-2001)
	struct addrinfo *localhost1, *localhost2;
	if(getaddrinfo("0.0.0.0", argv[1], 0, &localhost1)) {
		return 2;
	}
	if(getaddrinfo("0.0.0.0", argv[2], 0, &localhost2)) {
		return 3;
	}
	
	int sock1 = make_server(localhost1);
	if(sock1 < 0) {
		return -sock1 + 3;
	}
	
	int sock2 = make_server(localhost2);
	if(sock2 < 0) {
		return -sock2 + 6;
	}
	
	freeaddrinfo(localhost1);
	freeaddrinfo(localhost2);
	
	while(1) {
		int cli1 = accept(sock1, 0, 0);
		if(cli1 < 0) {
			return 10;
		}
		int cli2 = accept(sock2, 0, 0);
		if(cli2 < 0) {
			return 11;
		}
		pid_t pid1 = fork();
		if(pid1 < 0) {
			return 12;
		}
		
		if(pid1) {
			// nothing
		} else {
			close(sock1);
			close(sock2);
			pump_main(cli1, cli2);
			return 0;
		}
		
		pid_t pid2 = fork();
		if(pid2 < 0) {
			kill(pid1, SIGKILL);
			return 13;
		}
		if(pid2) {
			close(cli1);
			close(cli2);
		} else {
			close(sock1);
			close(sock2);
			pump_main(cli2, cli1);
			return 0;
		}
	}
	return 0;
}
