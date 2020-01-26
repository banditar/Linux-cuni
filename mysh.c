#include "headers.h"
#define ARG_MAX 2097152

struct entry {
    char command[ARG_MAX];
    STAILQ_ENTRY(entry)
    entries;
} *p;
STAILQ_HEAD(stailhead, entry) head = STAILQ_HEAD_INITIALIZER(head);


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
    return (NULL);
}

/*
 * checks for syntax errors
 * and returns the errored char
 */
char errorCheck(char *buf) {
    if (strchr(buf, '\\') != NULL) {
        return ('\\');
    }
    if (strchr(buf, '\'') != NULL) {
        return ('\'');
    }
    if (strchr(buf, '\"') != NULL) {
        return ('\"');
    }
    if (strchr(buf, '`') != NULL) {
        return ('`');
    }

    if (buf[0] == ';') {
        return (';');
    }

    if (buf[0] == '|' || buf[strlen(buf) - 1] == '|') {
        return ('|');
    }

    if (strstr(buf, ">>>") != NULL || strstr(buf, "<>") != NULL || strstr(buf, "><") != NULL) {
        return ('>');
    }

    if (strstr(buf, "<<") != NULL) {
        return ('<');
    }

    int i = 1;
    int colon = 0;
    int pipe = 0;
    while (buf[i] != '\0') {
        if (buf[i] == ';') {
            colon++;
            if (colon > 1) {
                return (';');
            }
        } else if (buf[i] == '|') {
            pipe++;
            if (pipe > 1) {
                return ('|');
            }
        } else if (buf[i] != ' ') {
            colon = 0;
            pipe = 0;
        }
        i++;
    }

    return (0);
}

/*
 * Trims spaces from front, back, middle
 * Cuts down comments
 */
char *trim(char *buf) {
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

    return (buf2);
}

/* Gives the index of the first separator in the string */
int firstSeparator(char *buf, char separator) {
    int i = 0;
    while (buf[i] != '\0' && buf[i] != separator) {
        i++;
    }
    return (i);
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
        char *oldpwd = getenv("PWD");
        char *pwd;
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
                return (1);
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
            return (1);
        }

        if (chdir(pwd) < 0) {
            if (row == 0) {
                fprintf(stderr, "mysh: cd: %s: No such file or directory\n", pwd);
            } else {
                fprintf(stderr, "mysh: error:%d: cd: %s: No such file or directory\n", row, pwd);
            }
            EXIT_VALUE = 1;
            return (1);
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
        return (0);
    }

    /* if not builtin */
    return (2);
}

/* boolean if pipe is present */
int pipe_bool = 0;
/* last command's pid in the pipe */
int last_pid = 0;
/* bool: if SIGINT was present in pipe */
int killed = 0;

/*
 * SIGINT signal handler for processes
 */
void sigint_process_handler(int sig) {
    fprintf(stderr, "Killed by signal 2.\n");
    if (pipe_bool == 0) {
        EXIT_VALUE = 130;
    } else {
        killed = 1;
    }
    (void) sig;        // silence the cc
    int wstatus;
    wait(&wstatus);
}

/*
 * Constructs the argument list for the execvp
 */
void arglist(char *buf, char *args[]) {
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
}

/*
 * Checks for redirections.
 * Opens the fileDescriptors.
 * dup2()-licates them, and closes them
 */
void
redirect(char *buf, int row) {
    // fd for input and output
    int fdin, fdout;
    // booleans
    int input = 0, output = 0, append = 0;
    // iterators
    int ini = 0, outi = 0;
    int LEN = strlen(buf);
    char IN[LEN], OUT[LEN];
    IN[0] = '\0';
    OUT[0] = '\0';

    // buf iterator
    long unsigned int i = 0;
    // line iterator
    long unsigned int j = 0;


    while (i < strlen(buf)) {
        if (buf[i] == '<') {
            // INPUT
            input = 1;
            ini = 0;
            // skip the <
            i++;

            while (i < strlen(buf) && buf[i] == ' ') {
                i++;
            }
            if (i < strlen(buf)) {
                // not error
                while (buf[i] != '\0'
                       && buf[i] != ' '
                       && buf[i] != '<'
                       && buf[i] != '>') {
                    // until string
                    // copy string into IN
                    IN[ini] = buf[i];
                    ini++;
                    i++;
                }
            }
            IN[ini] = '\0';
        }
        if (buf[i] == '>') {
            if (buf[i + 1] == '>') {
                // APPEND
                append = 1;
                output = 0;
                outi = 0;
                // skip the >>
                i += 2;

                while (i < strlen(buf) && buf[i] == ' ') {
                    i++;
                }
                if (i < strlen(buf)) {
                    // not error
                    while (buf[i] != '\0'
                           && buf[i] != ' '
                           && buf[i] != '<'
                           && buf[i] != '>') {
                        // until string
                        // copy string into OUT
                        OUT[outi] = buf[i];
                        outi++;
                        i++;
                    }
                }
                OUT[outi] = '\0';
            } else {
                // OUTPUT
                output = 1;
                append = 0;
                outi = 0;
                // skip the >
                i++;

                while (strlen(buf) > i && buf[i] == ' ') {
                    i++;
                }
                if (i < strlen(buf)) {
                    // not error
                    while (buf[i] != '\0'
                           && buf[i] != ' '
                           && buf[i] != '<'
                           && buf[i] != '>') {
                        // until string
                        // copy string into OUT
                        OUT[outi] = buf[i];
                        outi++;
                        i++;
                    }
                }
                OUT[outi] = '\0';
            }
        } else {
            buf[j] = buf[i];
            i++;
            j++;
        }
    }
    buf[j] = '\0';

    // opening fd-s
    if (input == 1 && strlen(IN) > 0) {
        if ((fdin = open(IN, O_RDONLY)) < 0) {
            if (row == 0) {
                fprintf(stderr, "mysh: %s: No such file or directory\n", IN);
            } else {
                fprintf(stderr, "error:%d: %s: No such file or directory\n", row, IN);
            }
        }
        if (dup2(fdin, 0) < 0) {
            fprintf(stderr, "Error dup2 IN: '%s'\n", IN);
        }
        close(fdin);
    }
    if (output == 1 && strlen(OUT) > 0) {
        if ((fdout = open(OUT, O_WRONLY | O_CREAT, 0666)) < 0) {
            fprintf(stderr, "Error opening OUT: '%s'\n", OUT);
        }
        if (dup2(fdout, 1) < 0) {
            fprintf(stderr, "Error dup2 OUT: '%s'\n", OUT);
        }

        close(fdout);
    }
    if (append == 1 && strlen(OUT) > 0) {
        if ((fdout = open(OUT, O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0) {
            fprintf(stderr, "Error opening APPEND: '%s'\n", OUT);
        }
        if (dup2(fdout, 1) < 0) {
            fprintf(stderr, "Error dup2 APPEND: '%s'\n", OUT);
        }

        close(fdout);
    }
}

/*
 * Executes the cmd given from the parse() function
 */
int execute(char *buf, int row) {
    pipe_bool = 0;
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
            return (1);
        }
        if (p == 0) { // child
            // sigaction(SIGINT, &sa, NULL);
            sigprocmask(SIG_SETMASK, &osigs, NULL);
            // check for redirections
            redirect(buf, row);

            char *args[1024];
            arglist(trim(buf), args);
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
            return (EXIT_VALUE);
        }
        EXIT_VALUE = WEXITSTATUS(wstatus);
        return (EXIT_VALUE);
    }
    return (ret);
}


/*
 * Checks what's in the Q. For debugging
 */
void
checkQ() {
    int i = 0;
    while (!STAILQ_EMPTY(&head)) {
        p = STAILQ_FIRST(&head);
        printf("pipe#%d: '%s'\n", i, p->command);
        i++;
        STAILQ_REMOVE_HEAD(&head, entries);
        free(p);
    }
}

/*
 * tokenizes the pipes and execs them
 */
int exec_pipe(char *buf, int row) {
    pipe_bool = 1;
    if (buf[0] == '|' || buf[strlen(buf) - 1] == '|') {
        /* if syntax error */
        if (row == 0) {
            fprintf(stderr, "syntax error near unexpected token '%c'\n", '|');
        } else {
            errx(254, "error:%d: syntax error near unexpected token '%c'", row, '|');
        }
        EXIT_VALUE = 254;
        return (254);
    }

    STAILQ_INIT(&head);

    /* count how many commands in the expression */
    int n = 0;
    char *line;
    int ret = 0;
    int nxtCommand = firstSeparator(buf, '|');
    while (buf[nxtCommand] != '\0') {
        line = nextCMD(buf, '|');
        p = malloc(sizeof(struct entry));
        strcpy(p->command, trim(line));
        STAILQ_INSERT_TAIL(&head, p, entries);
        n++;

        /* skip this command + one |*/
        buf += nxtCommand + 1;
        if (buf[0] == ' ') {
            /* skip space */
            buf++;
        }

        nxtCommand = firstSeparator(buf, '|');
    }

    p = malloc(sizeof(struct entry));
    strcpy(p->command, trim(buf));
    STAILQ_INSERT_TAIL(&head, p, entries);
    n++;

    sigset_t sigs, osigs;
    struct sigaction sa;
    sigfillset(&sigs);
    sigprocmask(SIG_BLOCK, &sigs, &osigs);
    sa.sa_handler = sigint_process_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    /* make the pipes */
    int **pip = (int **) malloc((n - 1) * sizeof(int *));

    // n child. (n - 1) pipes
    for (int i = 0; i < n - 1; i++) {
        pip[i] = (int *) malloc(2 * sizeof(int));
        if (pipe(pip[i]) < 0) {
            errx(1, "pipe error");
        }
    }

    int pid = fork();
    if (pid < 0) {
        errx(1, "fork error");
    }
    if (pid == 0) {
        // first child
        for (int i = 1; i < n - 1; i++) {
            close(pip[i][0]);
            close(pip[i][1]);
        }
        if (n > 1) {
            close(pip[0][0]);
        }
        if (n > 1) {
            if (dup2(pip[0][1], 1) < 0) {
                errx(1, "dup2 error");
            }
            close(pip[0][1]);
        }

        /* get the command out of the pipe */
        p = STAILQ_FIRST(&head);

        sigprocmask(SIG_SETMASK, &osigs, NULL);

        // check for redirections
        redirect(p->command, row);

        char *args[1024];
        arglist(trim(p->command), args);

        if (execvp(args[0], args) < 0) {
            EXIT_VALUE = 127;
            if (row == 0) {
                errx(127, "%s: command not found", p->command);
            } else {
                errx(127, "error:%d: %s: command not found", row, p->command);
            }
        }
        free(p);
        exit(ret);
    }

    for (int i = 1; i < n - 1; i++) {
        if ((pid = fork()) < 0) {
            errx(1, "fork error");
        }
        if (pid == 0) {
            // child
            for (int j = 0; j < n - 1; j++) {
                if (j != i) {
                    close(pip[j][1]);
                } else if (j != i - 1) {
                    close(pip[j][0]);
                }
            }
            if (dup2(pip[i - 1][0], 0) < 0) {
                errx(1, "dup2 error%d", i);
            }
            close(pip[i - 1][0]);

            if (dup2(pip[i][1], 1) < 0) {
                errx(1, "dup2 error%d", i);
            }
            close(pip[i][1]);

            /* get the command out of the pipe */
            for (int k = 0; k < i; k++) {
                p = STAILQ_FIRST(&head);
                STAILQ_REMOVE_HEAD(&head, entries);
                free(p);
            }
            p = STAILQ_FIRST(&head);

            sigprocmask(SIG_SETMASK, &osigs, NULL);

            // check for redirections
            redirect(p->command, row);

            char *args[1024];
            arglist(trim(p->command), args);

            if (execvp(args[0], args) < 0) {
                EXIT_VALUE = 127;
                if (row == 0) {
                    errx(127, "%s: command not found", p->command);
                } else {
                    errx(127, "error:%d: %s: command not found", row, p->command);
                }
            }

            fprintf(stderr, "%s executed: %d\n", p->command, ret);
            free(p);
            exit(ret);
        }
    }

    if (n > 1) {
        if ((pid = fork()) < 0) {
            errx(1, "fork error Last");
        }
        last_pid = pid;
        if (pid == 0) {
            // last child
            for (int i = 0; i < n - 1; i++) {
                if (i != n - 2) {
                    close(pip[i][0]);
                }
                close(pip[i][1]);
            }

            if (dup2(pip[n - 2][0], 0) < 0) {
                errx(1, "dup2 errorLast");
            }
            close(pip[n - 2][0]);

            /* get the commands out of the pipe */
            for (int k = 0; k < n - 1; k++) {
                p = STAILQ_FIRST(&head);
                STAILQ_REMOVE_HEAD(&head, entries);
                free(p);
            }
            p = STAILQ_FIRST(&head);

            sigprocmask(SIG_SETMASK, &osigs, NULL);

            // check for redirections
            redirect(p->command, row);

            char *args[1024];
            arglist(trim(p->command), args);

            if (execvp(args[0], args) < 0) {
                EXIT_VALUE = 127;
                if (row == 0) {
                    errx(127, "%s: command not found", p->command);
                } else {
                    errx(127, "error:%d: %s: command not found", row, p->command);
                }
            }

            free(p);
            exit(ret);
        }
    }

    for (int i = 0; i < n - 1; i++) {
        close(pip[i][0]);
        close(pip[i][1]);
    }

    sigprocmask(SIG_SETMASK, &osigs, NULL);
    sigaction(SIGINT, &sa, NULL);

    int wstatus;
    if (waitpid(pid, &wstatus, 0) < 0) {
        /* SIGINT-ed */
        EXIT_VALUE = 130;
        ret = EXIT_VALUE;
    } else {
        ret = WEXITSTATUS(wstatus);
        EXIT_VALUE = ret;
    }

    while (wait(&wstatus) >= 0);

    return (ret);
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
        return (254);
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
        if (strchr(buf, '|') != NULL) {
            killed = 0;
            ret = exec_pipe(trim(buf), row);
            if (killed == 1) {
                /* i.e. SIGINT-ed in pipe */
                return (ret);
            }
        } else {
            ret = execute(trim(buf), row);
        }

        if (ret == 130) {
            /* killed by SIGINT */
            /* Don't exec more */
            return (ret);
        }

        /* skip this cmd */
        line += nxtColon + 1;
        if (line[0] == ' ') {
            /* skip a space */
            line++;
        }

        nxtColon = firstSeparator(line, ';');
    }

    if (strchr(line, '|') != NULL) {
        ret = exec_pipe(trim(line), row);
    } else {
        ret = execute(trim(line), row);
    }
    return (ret);
}

/*
 * SIGINT signal handler for readline
 */
void
sigint_readline_handler(int sig) {
    printf("\n");           // Move to a new line
    rl_on_new_line();       // Regenerate the prompt on a newline
    rl_replace_line("", 0); // Clear the previous text
    rl_redisplay();
    (void) sig;             // To silence the cc
}

int
main(int argc, char **argv) {
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
        // int ARG_MAX = 2097152;
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
        char line[200];            // pwd to display for readline
        while (1) {
            sigaction(SIGINT, &sa, NULL);
            sprintf(line, "mysh: %s$ ", getenv("PWD"));

            buf = readline(line);
            if (buf != NULL) {
                if (strlen(buf) > 0) {
                    add_history(buf);
                }
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
    return (ret);
}