#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <err.h>

int
main() {
	struct pollfd fds[1];
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	int ret;
	int timeout = 1000;
	int k;
	while((ret = poll(fds, 1, timeout)) >= 0) {
		char buf[1024] = {0};
		if (fds[0].revents && POLLIN) {
			if((k = read(0, buf, 1024)) < 0) {
				errx(1, "Error");
			}
		}
		if(ret > 0) {
			write(1, buf, k);
		} else {
			printf("Timed out after %d\n", timeout);
		}
	}
}
