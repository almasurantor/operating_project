CC=gcc
CFLAGS=-Wall

all: shell

shell: shell.c
	$(CC) $(CFLAGS) shell.c -o shell

clean:
	rm -f shell
