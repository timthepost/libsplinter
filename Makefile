CC ?= gcc
AR ?= ar
CFLAGS := -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -fPIC -I3rdparty/
PREFIX ?= /usr/local

# Library objects
SHARED_LIBS = libsplinter.so libsplinter_p.so
STATIC_LIBS = libsplinter.a libsplinter_p.a
SHARED_HEADERS = splinter.h

# Good enough for now (if the CLI gets appreciably large, it needs a directory)
CLI_SOURCES := $(shell echo splinter_cli_*.c) 3rdparty/linenoise.c
CLI_HEADERS := splinter_cli.h 3rdparty/linenoise.h

# Helpers & tests
BIN_PROGS = splinter_recv splinter_send splinter_logtee splinter_stress splinter_cli
TESTS = splinter_test

# Default target
all: $(SHARED_LIBS) $(STATIC_LIBS) $(BIN_PROGS) $(TESTS)

# Object build
splinter.o: splinter.c splinter.h
	$(CC) $(CFLAGS) -c splinter.c -o $@

# Memory-backed shared object
libsplinter.so: splinter.o
	$(CC) -shared -Wl,-soname,libsplinter.so -o $@ $^

# Memory-backed static library
libsplinter.a: splinter.o
	$(AR) rcs $@ $^

# Persistent-mode object and shared object
splinter_p.o: splinter.c splinter.h
	$(CC) $(CFLAGS) -DSPLINTER_PERSISTENT -c splinter.c -o $@

libsplinter_p.so: splinter_p.o
	$(CC) -shared -Wl,-soname,libsplinter_p.so -o $@ $^

# Persistent-mode static library
libsplinter_p.a: splinter_p.o
	$(AR) rcs $@ $^

# Test binary (uses memory-backed .so by default)
splinter_test: splinter_test.c splinter.o
	$(CC) $(CFLAGS) -o $@ splinter_test.c splinter.o

# A testing tool to block for exactly one message on the bus
splinter_recv: splinter_recv.c splinter.o
	$(CC) $(CFLAGS) -o $@ splinter_recv.c splinter.o

# A testing tool to send a single message on [key] with [value] on the bus.
splinter_send: splinter_send.c splinter.o
	$(CC) $(CFLAGS) -o $@ splinter_send.c splinter.o

# A bus-to-file tap (WIP)
splinter_logtee: splinter_logtee.c splinter.o
	$(CC) $(CFLAGS) -o $@ splinter_logtee.c splinter.o

# Useful for actually testing libsplinter performance
splinter_stress: splinter_stress.c splinter.o
	$(CC) $(CFLAGS) -o $@ splinter_stress.c splinter.o

# A basic, but extensible CLI to navigate and manage stores
splinter_cli: $(CLI_SOURCES) $(CLI_HEADERS) splinter.o splinter.h
	$(CC) $(CFLAGS) -o $@ $(CLI_SOURCES) splinter.o

# Rust bindings; Deno bindings aren't quite automate-able yet.
.PHONY: rust_bindings

rust_bindings: libsplinter.so
	cd bindings/rust && cargo build

# Better experience if non-root tries root targets
.PHONY: be_root

be_root:
	@if [ "$$(id -u)" != "0" ]; then \
		echo "Error: Try that again, but as root." >&2; \
		/bin/false >/dev/null 2>&1; \
	fi

# Install artifacts
.PHONY: install

install: be_root
	install -m 0755 $(SHARED_LIBS) $(PREFIX)/lib/
	install -m 0644 $(STATIC_LIBS) $(PREFIX)/lib/
	install -m 0755 $(BIN_PROGS) $(PREFIX)/bin/
	install -m 0644 $(SHARED_HEADERS) $(PREFIX)/include/
	ldconfig

# Un-install artifacts
.PHONY: uninstall

uninstall: be_root
	rm -f $(PREFIX)/lib/$(SHARED_LIBS)
	rm -f $(PREFIX)/lib/$(STATIC_LIBS)
	rm -f $(PREFIX)/bin/$(BIN_PROGS)
	rm -f $(PREFIX)/include/$(SHARED_HEADERS)
	ldconfig

# Clean artifacts
.PHONY: clean

clean:
	rm -f $(BIN_PROGS) $(SHARED_LIBS) $(STATIC_LIBS) $(TESTS)
	rm -f *.o

# Clean artifacts and bindings
.PHONY: distclean

distclean: clean
	rm -rf bindings/rust/target
	rm -f  bindings/rust/Cargo.lock
	rm -f  bindings/rust/src/bindings.rs

# Tests
.PHONY: tests

tests: splinter_test splinter_stress
	./splinter_test
	@echo ""
	@echo "You can run ./splinter_stress to run stress tests."
	@echo "See ./splinter_stress --help for more."
	@echo ""
	@echo "You can/should also run tests under valgrind if you have it installed."
	@echo "Enable via HAVE_VALGRIND_H in config.h if you haven't already."
