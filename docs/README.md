# Libsplinter Developer Docs

`libsplinter` is a high-performance, lock-free, shared-memory key-value store
and message bus. It is written in C and designed for extremely fast
inter-process communication (IPC) on Linux systems. Its primary goal is to
facilitate the creation of microkernel-like process communities, especially for
orchestrating local Large Language Model (LLM) runtimes and their associated
services.

## Core Concepts

`libsplinter` is built on a few simple but powerful ideas:

- **Shared Memory**: It creates a segment of memory (either in `/dev/shm` for
  temporary, high-speed access or as a persistent file) that multiple
  independent processes can map and access directly. This avoids the overhead of
  traditional IPC methods like sockets or pipes.
- **Key-Value Store**: At its heart, it's a hash table. You can `set` a value
  for a given `key` and any other process can instantly `get` that value.
- **Atomic Operations**: It uses C11 atomics for managing access to data,
  preventing corruption when multiple processes read and write simultaneously
  without needing traditional locks (like mutexes), which simplifies usage and
  improves performance.
- **Publish/Subscribe (Pub/Sub)**: The library includes a `splinter_poll`
  function that allows a process to "subscribe" to a key and wait efficiently
  until another process updates it. This turns the simple key-value store into a
  powerful message bus.
- **Unopinionated**: You can use any wire protocol you want: just write it over
  the bus instead of over a socket. Or, build your own from the base envelope
  protocol that is included with this documentation: `base-protocol.md`, which is
  the basis of the author's LLM runtime.

## Building

To use `libsplinter`, you compile the C source into an object file and link it
with your application.

On most systems with a GNU development toolchain (e.g. Debian, Fedora):

```sh
make
```

To generate Rust bindings automatically, type:

```sh
make rust_bindings
```

Deno bindings are kept up-to-date with any public header updates through a manual
process I hope to automate at some point. PRs welcome to sort that out (and add 
more proper tests).

There is a `make install` option you can use in a chroot or container.

---

## Basic Usage: Key-Value Store

Here's how two processes can share data.

### `writer.c` - Creates a store and writes a value

```c
#include <stdio.h>
#include <string.h>
#include "splinter.h"

int main() {
    // Create a store named "my_store" with 128 slots
    // and a max value size of 256 bytes.
    // Or, open it if it already exists.
    if (splinter_create_or_open("my_store", 128, 256) != 0) {
        perror("splinter_create_or_open");
        return 1;
    }

    const char* key = "user:prompt";
    const char* value = "Tell me a joke about computers.";
    
    printf("Setting key '%s'\n", key);
    
    // Set the value for the key
    if (splinter_set(key, value, strlen(value)) != 0) {
        fprintf(stderr, "Failed to set key\n");
    }

    splinter_close();
    return 0;
}
```

### `reader.c` - Opens the store and reads the value

```c
#include <stdio.h>
#include <stdlib.h>
#include "splinter.h"

int main() {
    // Open the existing store
    if (splinter_open("my_store") != 0) {
        perror("splinter_open");
        return 1;
    }

    const char* key = "user:prompt";
    char buffer[256];
    size_t value_len;

    // Get the value for the key
    if (splinter_get(key, buffer, sizeof(buffer), &value_len) == 0) {
        // Add a null terminator to make it a printable string
        buffer[value_len] = '\0';
        printf("Read key '%s' -> '%s'\n", key, buffer);
    } else {
        fprintf(stderr, "Failed to get key '%s'\n", key);
    }
    
    splinter_close();
    return 0;
}
```

## Advanced Usage: Message Bus (Pub/Sub)

The real power for building responsive systems comes from `splinter_poll`. One
process can wait for a notification that data has changed.

### `subscriber.c` - Waits for an update

```c
#include <stdio.h>
#include <stdlib.h>
#include "splinter.h"

int main() {
    // Open or create the store
    if (splinter_open_or_create("llm_bus", 128, 1024) != 0) {
        perror("splinter_open_or_create");
        return 1;
    }
    
    const char* key = "llm:response";
    printf("Waiting for response on key '%s'...\n", key);

    // Wait up to 60 seconds for the key to be updated
    if (splinter_poll(key, 60000) == 0) {
        printf("Update received!\n");
        char buffer[1024];
        size_t len;
        if (splinter_get(key, buffer, sizeof(buffer), &len) == 0) {
            buffer[len] = '\0';
            printf("Response: %s\n", buffer);
        }
    } else {
        fprintf(stderr, "Poll timed out or key does not exist.\n");
    }

    splinter_close();
    return 0;
}
```

### `publisher.c` - Publishes the update

```c
#include <stdio.h>
#include <string.h>
#include <unistd.h> // for sleep()
#include "splinter.h"

int main() {
    if (splinter_open_or_create("llm_bus", 128, 1024) != 0) {
        perror("splinter_open_or_create");
        return 1;
    }
    
    const char* key = "llm:response";
    const char* response = "Why was the computer cold? It left its Windows open.";

    printf("Simulating work for 3 seconds...\n");
    sleep(3);
    
    printf("Publishing response to key '%s'\n", key);
    if (splinter_set(key, response, strlen(response)) != 0) {
        fprintf(stderr, "Failed to set key\n");
    }

    splinter_close();
    return 0;
}
```

---

## API Reference

### Setup and Teardown

- `int splinter_create(const char *name, size_t slots, size_t max_val_sz)`
  Creates a new store. Fails if it already exists.
- `int splinter_open(const char *name)` Opens an existing store. Fails if it
  doesn't exist.
- `int splinter_create_or_open(const char *name, ...)` Creates a store, or opens
  it if it exists.
- `int splinter_open_or_create(const char *name, ...)` Opens a store, or creates
  it if it doesn't exist.
- `void splinter_close(void)` Closes the store and unmaps memory.

### Core Operations

- `int splinter_set(const char *key, const void *val, size_t len)` Sets or
  updates a key with a new value.
- `int splinter_get(const char *key, void *buf, size_t buf_sz, size_t *out_sz)`
  Retrieves a value by its key.
- `int splinter_list(char **out_keys, size_t max_keys, size_t *out_count)` Fills
  an array with pointers to all keys in the store.

### Pub/Sub

- `int splinter_poll(const char *key, uint64_t timeout_ms)` Blocks until the
  specified key is updated by a `splinter_set` call, or until the timeout is
  reached.
