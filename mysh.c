#include "headers.h"

/*
 * returns the first CMD from buf.
 * I.e. before first separator (e.g. ';')
 */
char *nextCMD(char *buf, char separator) {
				int i = 0;
				if (strchr(buf, separator) != NULL) {
								while (buf[i] != '\0' && buf[i] != separator) {
												i++;
								}
								buf[i] = '\0';
								return (buf);
				}
				return NULL;
}

/*
 * checks for syntax errors
 * and returns the errored char
 */
char errorCheck(char *buf) {
				if (strchr(buf, '\\') != NULL) {
								return '\\';
				}
				if (strchr(buf, '\'') != NULL) {
								return '\'';
				}
				if (strchr(buf, '\"') != NULL) {
								return '\"';
				}
				if (strchr(buf, '`') != NULL) {
								return '`';
				}

				if (buf[0] == ';') {
								return ';';
				}

				int i = 1;
				int colon = 0;
				while (buf[i] != '\0') {
								if (buf[i] == ';') {
												colon++;
												if (colon > 1) {
																return ';';
												}
								} else if (buf[i] != ' ') {
												colon = 0;
								}
								i++;
				}

				return 0;
}

/*
 * Trims spaces from front, back, middle
 * Cuts down comments
 */
char* trim(char *buf) {
				// comments
				int i = 0;
				while (buf[i] != '\0') {
								if (buf[i] == '#') {
												buf[i] = '\0';
								} else {
												i++;
								}
				}

				// from front
				i = 0;
				while (buf[i] != '\0' && buf[i] == ' ') {
								buf++;
				}

				// from behind
				i = strlen(buf) - 1;
				while (i >= 0 && buf[i] == ' ') {
								i--;
				}
				buf[i + 1] = '\0';

				// from the middle
				int space = 0;
				char *buf2 = buf;
				int j = 0;
				i = 0;
				while (buf[i] != '\0') {
								if (buf[i] == ' ') {
												if (space == 0) {
																buf2[j] = buf[i];
																j++;
												}
												space++;
								} else {
												buf2[j] = buf[i];
												j++;
												space = 0;
								}
								i++;
				}
				buf2[j] = '\0';

				return buf2;
}

/* Gives the index of the first separator in the string */
int firstSeparator(char *buf, char separator) {
				int i = 0;
				while (buf[i] != '\0' && buf[i] != separator) {
								i++;
				}
				return i;
}

int EXIT_VALUE = 0;

/*
 * checks for builtin functions and executes them
 * I.e. exit and cd commands
 */
int builtins(char buf[], int row) {
				if (strcmp(buf, "exit") == 0) {
								exit(EXIT_VALUE);
				}
				if (buf[0] == 'c' && buf[1] == 'd') {
								char* oldpwd = getenv("PWD");
								char* pwd;
								if (strlen(buf) <= 2) {
												/* cd goes to home */
												pwd = getenv("HOME");
								} else if (buf[3] == '-') {
												/* cd goes back one dir */
												pwd = getenv("OLDPWD");
												if (pwd == NULL) {
																/* wasn't changed yet */
																if (row == 0) {
																				fprintf(stderr, "mysh: cd: OLDPWD not set\n");
																} else {
																				fprintf(stderr, "mysh: error:%d: cd: OLDPWD not set\n", row);
																}
																EXIT_VALUE = 1;
																return 1;
												}
								} else {
												pwd = buf + 3;
								}
								/* chdir */
								if (strchr(pwd, ' ') != NULL) {
												/* if too many args */
												if (row == 0) {
																fprintf(stderr, "mysh: cd: too many arguments\n");
												} else {
																fprintf(stderr, "mysh: error:%d: cd: too many arguments\n", row);
												}
												EXIT_VALUE = 1;
												return 1;
								}

								if (chdir(pwd) < 0) {
												if (row == 0) {
																fprintf(stderr, "mysh: cd: %s: No such file or directory\n", pwd);
												} else {
																fprintf(stderr, "mysh: error:%d: cd: %s: No such file or directory\n", row, pwd);
												}
												EXIT_VALUE = 1;
												return 1;
								}

								/* setOLDPWD */
				        if (setenv("OLDPWD", oldpwd, 1) < 0) {
												errx(1, "SetOLDPWD error");
								}
								
								/* setPWD */
								if (setenv("PWD", pwd, 1) < 0) {
												errx(1, "SetPWD error");
								}
								EXIT_VALUE = 0;
								return 0;
				}

				/* if not builtin */
				return 2;
}

/*
 * SIGINT signal handler for processes
 */
void sigint_process_handler(int sig) {
				fprintf(stderr, "Killed by signal 2.\n");
				EXIT_VALUE = 130;
				(void) sig;		// silence the cc
				wait(NULL);
}

/*
 * Executes the cmd given from the parse() function
 */
int execute(char *buf, int row) {
				int ret = builtins(buf, row);
				if (ret == 2) {
								// not 'exit' nor 'cd'
								sigset_t sigs, osigs;
								struct sigaction sa;
								sigfillset(&sigs);
								sigprocmask(SIG_BLOCK, &sigs, &osigs);
								sa.sa_handler = sigint_process_handler;
								sigemptyset(&sa.sa_mask);
				        sa.sa_flags = 0;

								int p;
								if ((p = fork()) < 0) {
												sigprocmask(SIG_SETMASK, &osigs, NULL);
												fprintf(stderr, "Fork error\n");
												EXIT_VALUE = 1;
												return 1;
								}
								if (p == 0) { // child
												// sigaction(SIGINT, &sa, NULL);
												sigprocmask(SIG_SETMASK, &osigs, NULL);
												char *args[1024];
												int i = 0;
												int nxtSpace = firstSeparator(buf, ' ');
												while (buf[nxtSpace] != '\0') {
																// first CMD before space
																args[i] = nextCMD(buf, ' ');
																// skip this CMD
																buf += nxtSpace + 1;
																i++;
																nxtSpace = firstSeparator(buf, ' ');
												}
												args[i] = buf;
												i++;
												args[i] = NULL;

												if (execvp(args[0], args) < 0) {
																EXIT_VALUE = 127;
																if (row == 0) {
																				errx(127, "%s: command not found", buf);
																} else {
																				errx(127, "error:%d: %s: command not found", row, buf);
																}
												}
								}
								sigprocmask(SIG_SETMASK, &osigs, NULL);
								sigaction(SIGINT, &sa, NULL);
								int wstatus;
								if (wait(&wstatus) == -1) {
												/* if SIGINT-ed */
												return EXIT_VALUE;
								}
								EXIT_VALUE = WEXITSTATUS(wstatus);
								return EXIT_VALUE;
				}
				return ret;
}

/*
 * parses the string given by the readline()
 * and execvp-s the cmds
 */
int parse(char *line, int row) {
				char error = errorCheck(line);
				if (error != 0) {
								EXIT_VALUE = 254;
								if (row == 0) {
												fprintf(stderr, "syntax error near unexpected token '%c'\n", error);
								} else {
												errx(254, "error:%d: syntax error near unexpected token '%c'", row, error);
								}
								EXIT_VALUE = 254;
								return 254;
				}

				int len = strlen(line);
				if (line[len - 1] == ';') {
								/* trim the ';' from the end */
								line[len - 1] = '\0';
				}

				char *buf;
				int ret;
				int nxtColon = firstSeparator(line, ';');
				while (line[nxtColon] != '\0') {
								buf = nextCMD(line, ';');

								/* where execvp is */
								ret = execute(trim(buf), row);

								if (ret == 130) {
												/* killed by SIGINT */
												/* Don't exec more */
												return ret;
								}

								/* skip this cmd */
								line += nxtColon + 1;
								if (line[0] == ' ') {
												/* skip a space */
												line++;
								}

								nxtColon = firstSeparator(line, ';');
				}

				ret = execute(trim(line), row);
				return ret;
}

/*
 * SIGINT signal handler for readline
 */
void
sigint_readline_handler(int sig) {
				printf("\n"); 					// Move to a new line
		    rl_on_new_line(); 			// Regenerate the prompt on a newline
		    rl_replace_line("", 0); // Clear the previous text
		    rl_redisplay();
				(void) sig;							// To silence the cc
}

int
main(int argc, char** argv) {
				int ret;
				int c = getopt(argc, argv, "c:");
				if (c != -1) {
								/* OPTION -c */
								if (c == 'c') {
												optarg = trim(optarg);
												ret = parse(optarg, 0);
								} else {
												errx(1, "Usage: %s [-c cmd] [file]", argv[0]);
								}
				} else if (argc > 1) {
								/* CMDS FROM FILE */
								int fd;
								if ((fd = open(argv[1], O_RDONLY)) == -1) {
												errx(127, "%s: No such file or directory", argv[1]);
								}
								int ARG_MAX = 2097152;
								int nrBytesRead = 1;
								int r;
								char *buf2 = (char *) malloc(ARG_MAX * sizeof(char));
								char c[1];
								int i = 0;
								int row = 1;

								while ((r = read(fd, c, nrBytesRead)) != 0) {
												if (r == -1) {
																close(fd);
																errx(1, "Read error: %s", argv[1]);
												}

												if (c[0] == '\n') {
																buf2[i] = '\0';
																buf2 = trim(buf2);
																if (strlen(buf2) > 0) {
																				ret = parse(buf2, row);
																}
																i = 0;
																row++;
												} else {
																buf2[i] = c[0];
																i++;
												}
								}
								free(buf2);
								close(fd);
				} else {
								/* INTERACTIVE MODE */
								/* setting SIGINT to clear the line */
				        struct sigaction sa;
        				sa.sa_handler = sigint_readline_handler;
				        sigemptyset(&sa.sa_mask);
		   		      sa.sa_flags = 0;

								char *buf;
								char line[200];			// pwd to display for readline
								while (1) {
												sigaction(SIGINT, &sa, NULL);
												sprintf(line, "mysh: %s$ ", getenv("PWD"));

												buf = readline(line);
												if (buf != NULL) {
																add_history(buf);
																char *buf2 = trim(buf);
																if (strlen(buf2) > 0) {
																				ret = parse(buf2, 0);
																}
												} else {
																/* ^D = exit */
																free(buf);
																printf("exit\n");
																exit(EXIT_VALUE);
												}
												free(buf);
								}
				}
				return ret;
}
