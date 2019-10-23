#include <stdio.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

int
main(int argc, char **argv) {
				int opt;
				char f[128];
				char c[128];
				int fd;
				while((opt = getopt(argc, argv, "abc::f:")) != -1) {
								if (opt == -1) {
												printf("End\n");
								}
								if (opt == 'a') {
												printf("option a\n");
								} else if (opt == 'b') {
												printf("option b\n");
								} else if (opt == 'f') {
												if (optopt == 'f') {
																errx(1, "option -f requires an argument\n");
												}

												strncpy(f, optarg, sizeof (f) - 1);
												f[sizeof(f) - 1] = '\0';
												printf("option f <%s>:\n", f);

												if ((fd = open(f, O_RDONLY)) <= 0) {
																errx(1, "Error opening file %s\n", f);
												}
												
												int len;
												int go = 0;
												while((len = read(fd, c, 128)) > 0) {
																int i = 0;
																int was = 0;
																
																while(c[i] != '\0') {			// harmas feladat
																				if (c[i] == '\n') {
																								poll(NULL, 0, 200);
																								was = 1;
																								go = 0;
																				}
																				if (go == 0) {
																								printf("%c", c[i]);
																				}
																				i++;
																}
																if (was == 0) {
																				go = 1;
																}
																
//																write(1, c, len);     // kettes feladat
												}

												close(fd);
								} else if (opt == 'c'){
												if (optopt == 'c') {
																errx(1, "option -c requires two files to copy\n");
												}


								} else {
												errx(1, "unknown option\nUsage: %s [-ab][-f filename][-c file1 file2]\n", argv[0]);
								}
				}

				return (0);
}
