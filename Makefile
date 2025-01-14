MAKEFLAGS += --silent
CC = gcc
CFLAGS = -Wall -lm
TARGET = build/script
SRCS = $(wildcard src/*.c)
OBJDIR = .cache
OBJS = $(SRCS:src/%.c=$(OBJDIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lm

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build: CFLAGS = -Wall -o3 -lm
build: $(TARGET)

check: CFLAGS = -Wall -lm -g -ggdb3
check: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=.cache/valgrind-out.txt ./$(TARGET) ./tests/test.klein

clean:
	rm .cache -rf
	mkdir .cache

test: $(TARGET)
	./$(TARGET) tests/test.klein

.PHONY: all clean check

