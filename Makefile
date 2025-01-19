CC = clang
LOCATION = /usr/bin
DEBUG = false

MAKEFLAGS += --silent

CACHEDIR = ./.cache
OBJDIR = $(CACHEDIR)/obj
VALGRINDDIR = $(CACHEDIR)/valgrind
TARGET = ./build/klein
TESTFILE = ./tests/test.kl

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=$(OBJDIR)/%.o)

# Clang flags
ifeq ($(CC), clang)
	CFLAGS = -ferror-limit=0 -fdiagnostics-color=always -Wall -Wextra -Weverything -Wno-padded -Wno-extra-semi-stmt -Wno-switch-default -Wno-unsafe-buffer-usage -Wno-declaration-after-statement -Wno-switch-enum
endif

# gcc flags
ifeq ($(CC), gcc)
	CFLAGS = -Wall -Wextra
endif

# Debug
ifeq ($(DEBUG), true)
	CFLAGS += -DDEBUG_ON
endif

# Build when just running `make`
all: build
	$(TARGET)

# Link & compile object files into native executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lm

# Compilation into object files
$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build the binary
build: clean $(TARGET)

# Build & run Valgrind to check for memory errors
check: CFLAGS += -g -ggdb3
check: build
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=.cache/valgrind/valgrind-out.txt $(TARGET) run $(TESTFILE)

# Clean cache files (object files, valgrind logs, etc.)
clean:
	rm $(CACHEDIR) -rf
	mkdir $(CACHEDIR)
	mkdir $(VALGRINDDIR)
	mkdir $(OBJDIR)
	rm build -rf
	mkdir build

# Run on the test file
test: build
	$(TARGET) $(TESTFILE)

# Run with debug mode
debug: CFLAGS += -DDEBUG_ON
debug: test

# Install on the system
install: build
	sudo mkdir -p $(LOCATION)
	sudo mv $(TARGET) $(LOCATION)/klein

.PHONY: all clean check build test install debug
