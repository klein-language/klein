CC = clang
CFLAGS = -ferror-limit=0 -fdiagnostics-color=always -Wall -Wextra
MAKEFLAGS += --silent

OBJDIR = ./obj
BUILDDIR = ./build
TARGET = $(BUILDDIR)/program
LIB = ./bindings/c/klein.a
SRCS = $(./*.c)
OBJS = $(SRCS:src/%.c=$(OBJDIR)/%.o)

# Build when just running `make`
all: build
	$(TARGET)

# Link & compile object files into native executable
$(TARGET): $(OBJS)
	$(CC) $(LIB) $(OBJS) -o $(TARGET)

# Compilation into object files
$(OBJDIR)/%.o: ./%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build the binary
build: clean $(TARGET)

# Clean cache files (object files, valgrind logs, etc.)
clean:
	rm $(OBJDIR)/*
	rm $(BUILDDIR)/*

# Run on the test file
test: build
	$(TARGET) $(TESTFILE)

.PHONY: all clean build test
