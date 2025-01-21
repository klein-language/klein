# Customizeable
CC = clang # The C compiler to use
LOCATION = /usr/bin # Where to install
EXE = klein # Name of the executable

# Probably don't change these
CACHEDIR = ./.cache
OBJDIR = $(CACHEDIR)/obj
VALGRINDDIR = $(CACHEDIR)/valgrind
BUILDDIR = ./build
TARGET = $(BUILDDIR)/$(EXE)
TESTFILE = ./tests/klein/test.kl

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=$(OBJDIR)/%.o)

MAKEFLAGS += --silent

# Clang flags
ifeq ($(CC), clang)
	CFLAGS = -ferror-limit=0 -fdiagnostics-color=always -Wall -Wextra -Weverything -Wno-padded -Wno-extra-semi-stmt -Wno-switch-default -Wno-unsafe-buffer-usage -Wno-declaration-after-statement -Wno-switch-enum -Wno-implicit-int-float-conversion -Wno-unused-macros -Wno-c++98-compat-pedantic
endif

# gcc flags
ifeq ($(CC), gcc)
	CFLAGS = -Wall -Wextra
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
	rm $(BUILDDIR) -rf
	mkdir $(BUILDDIR)

# Run on the test file
test: build
	$(TARGET) $(TESTFILE)

# Install on the system
install: build
	sudo mkdir -p $(LOCATION)
	sudo mv $(TARGET) $(LOCATION)/$(EXE)

# Build static library
c-bindings: $(OBJS)
	ar rcs ./bindings/c/klein.a $(OBJDIR)/*.o
	$(CC) -shared -fPIC -o ./bindings/c/libklein.so $(SRCS)

rust-bindings: c-bindings
	cd bindings/rust; cargo build; cargo test -- --nocapture

bindings: rust-bindings

.PHONY: all clean check build test install package
