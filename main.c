#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

int
main() {
	int fdnr = 2;
	struct pollfd fds[fdnr];
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	struct sockaddr_in in = {0};

	in.sin_family = AF_INET;
	int port = 2222;
	printf("Using port: %d\n", port);
	in.sin_port = htons(port);
	in.sin_addr.s_addr = htonl(INADDR_ANY);

	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		errx(1, "socket");
	}

	int optval = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
	    &optval, sizeof(optval)) == -1) {
	    	errx(1, "setsockopt");
	}

	if (bind(fd, (struct sockaddr *)&in, sizeof (in)) == -1)
		errx(1, "bind");

	if (listen(fd, SOMAXCONN) == -1)
		errx(1, "listen");

	fds[1].fd = fd;
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	int ret;
	int timeout = 10000;
	int k;
	while((ret = poll(fds, fdnr, timeout)) >= 0) {
		char buf[3] = {0};
		if (fds[0].revents && POLLIN) {
			if((k = read(0, buf, sizeof(buf))) < 0) {
				errx(1, "Error");
			}
		
			write(1, buf, k);
		}

		// tcp
		if( fds[1].revents && POLLIN) {
			int newsock;
			if ((newsock = accept(fd, NULL, 0)) == -1) {
				errx(1, "accept");
			}

			printf("--Connection established--\n");
			
			close(newsock);
			printf("--Connection closed--\n");
		}

		if (ret == 0) {
			printf("Timed out after %d\n", timeout);
		}
	}
}
