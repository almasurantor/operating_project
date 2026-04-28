CC       = gcc
CFLAGS   = -Wall -Wextra -pedantic -std=c11 -O2
LDFLAGS  = -lpthread
SRCS     = $(wildcard *.c)
OBJS     = $(SRCS:.c=.o)
TARGET   = filediffadvanced

.PHONY: all clean test man

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	bash run_tests.sh

man:
	man ./filediffadvanced.1
