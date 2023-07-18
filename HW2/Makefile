CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -g

SRCS = hw2.c
OBJS = $(SRCS:.c=.o)
EXEC = hw2

.PHONY: all clean valgrind

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(filter-out hw2.c report.pdf Makefile, $(wildcard *))

valgrind: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all ./$(EXEC)
