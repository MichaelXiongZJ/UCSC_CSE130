CC=clang
CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic -Wno-unused-parameter -g
LDFLAGS=-lpthread

all: example1 example2 example3

example1: example1.o dining.o utils.o
example2: example2.o dining.o utils.o
example3: example3.o dining.o utils.o

%.o : %.c
	$(CC) $(CFLAGS) $< -c

.PHONY: clean
clean:
	- rm -f *.o example1 example2 example3

.PHONY: format
format:
	clang-format -i *.c *.h

.PHONY: check-format
check-format:
	clang-format --dry-run --Werror *.c *.h
