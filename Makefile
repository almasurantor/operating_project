CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2

.PHONY: all clean shell timedexec loganalyzer filecrypt filediffadvanced

all: shell timedexec loganalyzer filecrypt filediffadvanced

shell: shell.c
	$(CC) $(CFLAGS) -o shell shell.c

timedexec:
	$(CC) $(CFLAGS) -o commands/timedexec/timedexec commands/timedexec/timedexec.c

loganalyzer:
	$(CC) $(CFLAGS) -o commands/loganalyzer/loganalyzer commands/loganalyzer/loganalyzer.c

filecrypt:
	$(CC) $(CFLAGS) -o commands/filecrypt/filecrypt commands/filecrypt/filecrypt.c

filediffadvanced:
	$(MAKE) -C commands/filediffadvanced

clean:
	rm -f shell
	rm -f commands/timedexec/timedexec
	rm -f commands/loganalyzer/loganalyzer
	rm -f commands/filecrypt/filecrypt
	$(MAKE) -C commands/filediffadvanced clean
