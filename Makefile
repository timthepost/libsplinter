CC ?= gcc
AR ?= ar
CFLAGS := -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -fPIC
PREFIX ?= /usr/local

# Library objects
SHARED_LIBS = libsplinter.so libsplinter_p.so
STATIC_LIBS = libsplinter.a libsplinter_p.a
SHARED_HEADERS = splinter.h

# Helpers & tests
BIN_PROGS = splinter_recv splinter_send splinter_logtee sidecar
TESTS = splinter_test

# Default target
all: $(SHARED_LIBS)  $(STATIC_LIBS) $(BIN_PROGS) $(TESTS)
	$(MAKE) -C shmbus/

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
	$(CC) $(CFLAGS) -o @@ splinter_logtee.c splinter.o

# A neat little tool to watch system utilization while squawking on a debug bus.
sidecar: sidecar.c splinter.o
	$(CC) $(CFLAGS) -o $@ sidecar.c splinter.o 

# Rust bindings; Deno bindings aren't quite automate-able yet.
.PHONY: rust_bindings

rust_bindings: libsplinter.so
	cd bindings/rust && cargo build

# Install artifacts
.PHONY: install

install:
	install -m 0755 $(SHARED_LIBS) $(PREFIX)/lib/
	install -m 0644 $(STATIC_LIBS) $(PREFIX)/lib/
	install -m 0755 $(BIN_PROGS) $(PREFIX)/bin/
	install -m 0644 $(SHARED_HEADERS) $(PREFIX)/include/

# Un-install artifacts
.PHONY: uninstall

uninstall:
	rm -f $(PREFIX)/lib/$(SHARED_LIBS)
	rm -f $(PREFIX)/lib/$(STATIC_LIBS)
	rm -f $(PREFIX)/bin/$(BIN_PROGS)
	rm -f $(PREFIX)/include/$(SHARED_HEADERS)

# Clean artifacts
.PHONY: clean

clean:
	rm -f $(BIN_PROGS) $(SHARED_LIBS) $(STATIC_LIBS) $(TESTS)
	rm -f *.o
	$(MAKE) -C shmbus/ clean

# Clean artifacts and bindings
.PHONY: distclean

distclean: clean
	rm -rf bindings/rust/target
	rm -f  bindings/rust/Cargo.lock
	rm -f  bindings/rust/src/bindings.rs
