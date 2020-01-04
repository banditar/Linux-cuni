file=mysh
CC=@cc -Wextra -Wall -std=c99 -lreadline

all:$(file) headers.h
	$(CC) $(file).c -o $(file)

.PHONY: all
