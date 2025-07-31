CC ?= gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -fPIC
AR ?= ar

SHARED_LIBS = libshmbus.so
STATIC_LIBS = libshmbus.a
TESTS = shmbus_test

all: $(SHARED_LIBS) $(STATIC_LIBS) $(TESTS)

shmbus.o: shmbus.c shmbus.h
	$(CC) $(CFLAGS) -c shmbus.c -o $@

libshmbus.so: shmbus.o
	$(CC) -shared -Wl,-soname,libshmbus.so -o $@ $^

libshmbus.a: shmbus.o
	$(AR) rcs $@ $^

shmbus_test: shmbus_test.c shmbus.o
	$(CC) $(CFLAGS) -o $@ shmbus_test.c shmbus.o

clean:
	rm -f *.o $(SHARED_LIBS) $(STATIC_LIBS) $(TESTS)

install:
	echo "This is not meant to be installed as is, but rather"
	echo "embedded in something else. Please see the README."
	exit 1
