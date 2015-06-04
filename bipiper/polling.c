#define _POSIX_C_SOURCE 201505
#define _GNU_SOURCE

#include <bufio.h>
#include <helpers.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define LISTEN_QUEUE 5

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

struct pollfd fds[256];
buf_t* bufs[256]; // hurr durr pair<T,U> is not in this language
int clients = 0;
int want_accept = 0;

void close_pair(int i) {
	if(clients == 254) {
		fds[want_accept].events = POLLIN;
	}
	close(fds[i + 2].fd);
	close(fds[(i + 2) ^ 1].fd);
	buf_free(bufs[i]);
	buf_free(bufs[i ^ 1]);
	if(clients > 3) {
		int fi = i & ~1;
		fds[fi + 2] = fds[clients];
		fds[fi + 3] = fds[clients + 1];
		bufs[fi] = bufs[clients - 2];
		bufs[fi + 1] = bufs[clients - 1];
	}
	clients -= 2;
}

int main(int argc, char** argv) {
	if(argc < 3) {
		write_(STDOUT_FILENO, "Usage: <port1> <port2>\n", 23);
		return 1;
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO); // totally don't need these three
	struct addrinfo *localhost1, *localhost2;
	if(getaddrinfo("localhost", argv[1], 0, &localhost1)) {
		return 2;
	}
	if(getaddrinfo("localhost", argv[2], 0, &localhost2)) {
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
	
	memset(fds, 0, sizeof(struct pollfd) * 256);
	memset(bufs, 0, sizeof(void*) * 256);
	
	fds[0].fd = sock1;
	fds[1].fd = sock2;
	
	fds[0].events = POLLIN;
	
	while(1) {
		int res = poll(fds, clients + 2, -1);
		if(res == -1) {
			if(errno == EINTR)
				continue;
			return 10;
		}
		short ce = fds[want_accept].revents;
		if(ce) {
			if(ce & POLLIN) {
				fds[want_accept].events = 0;
				fds[want_accept].revents = 0;
				int cli = accept(fds[want_accept].fd, 0, 0);
				if(cli < 0) {
					return 11;
				}
				bufs[clients] = buf_new(BUFFER_SIZE);
				fds[clients + 2].events = POLLIN | POLLRDHUP;
				fds[clients + 2].fd = cli;
				clients++;
				want_accept ^= 1;
				if(clients < 254) {
					fds[want_accept].events = POLLIN;
				}
			} else {
				return 12;
			}
		}
		for(int i = 0; i < (clients & ~1); i++) {
			ce = fds[i + 2].revents;
			fds[i + 2].revents = 0;
			if(ce) {
				if(ce & POLLOUT) {
					size_t oldsize = buf_size(bufs[i ^ 1]);
					buf_flush(fds[i + 2].fd, bufs[i ^ 1], 1);
					if(buf_size(bufs[i ^ 1]) == 0) {
						fds[i + 2].events &= ~POLLOUT;
					}
					if(oldsize == buf_capacity(bufs[i ^ 1]) && buf_size(bufs[i ^ 1]) < buf_capacity(bufs[i ^ 1])) {
						fds[(i + 2) ^ 1].events |= POLLIN;
					}
				}
				if(ce & POLLIN) {
					size_t oldsize = buf_size(bufs[i]);
					buf_fill(fds[i + 2].fd, bufs[i], buf_size(bufs[i]) + 1);
					if(buf_size(bufs[i]) == buf_capacity(bufs[i])) {
						fds[i + 2].events &= ~POLLIN;
					}
					if(oldsize == 0 && buf_size(bufs[i]) > 0) {
						fds[(i + 2) ^ 1].events |= POLLOUT;
					}
				}
				if(ce & POLLRDHUP) {
					shutdown(fds[(i + 2) ^ 1].fd, SHUT_WR);
					fds[(i + 2) ^ 1].events &= ~POLLOUT;
					if(~fds[i + 2].events & ~fds[(i + 2) ^ 1].events) {
						close_pair(i);
						i |= 1;
						continue;
					}
				}
				if(ce & POLLHUP) {
					shutdown(fds[(i + 2) ^ 1].fd, SHUT_RD);
					fds[(i + 2) ^ 1].events &= ~POLLIN;
					if(~fds[i + 2].events & ~fds[(i + 2) ^ 1].events) {
						close_pair(i);
						i |= 1;
						continue;
					}
				}
				if(ce & POLLERR) {
					i |= 1;
					close_pair(i);
				}
			}
		}
	}
	return 0;
}
