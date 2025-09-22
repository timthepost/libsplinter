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

## Advanced Usage - Auto Vacuum (Auto-scrubbing)

Libsplinter uses a single value region and stores offsets for recalling specific values by key. Value slots can be up to a configurable length, typically 4k. By default, splinter zeros out the entire 4k prior to every update, so that writes that are smaller than the current occupied size don't leave fragments old values behind, becaus they aren't large enough to overwrite them completely. 

This is the default because, for LLM workloads, even a slight nonzero chance of stale data leaking back into active keys and corrupting training or context is unnacceptable. The circumstances under which it can happen aren't even a supported use case, but if you have any kind of synchronization issues where threads flip from being readers to writers erroneously, it becomes likely enough that I coded defensively against it.

So, in "Facebook / Anthropic" scale levels (which we're not even designed to serve), you can turn
off the extra memset() and get zero torn read guarantee instead.

To do this, just:

`splinter_set_av(0);`

See `splinter_stress.c` for more. 

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

### Bus Management
 - `int splinter_set_av(unsigned int mode)` Sets the auto vacuum mode of the current
   bus to `mode` (0 = off, 1 = on, default = 1). See the docs prior to changing this.
 - `int splinter_get_av(void)` Gets the (atomic) value of the auto vacuum toggle. 

### Pub/Sub

- `int splinter_poll(const char *key, uint64_t timeout_ms)` Blocks until the
  specified key is updated by a `splinter_set` call, or until the timeout is
  reached.

#### Adding Additional Feature Flags:

Right now we only have one feature, so there's no reason to build up around having many, but if the need arises there is a plan (which you can implement yourself if you need it right now):

Currently, Splinter's global bus data structure looks like this:

```c
struct splinter_header {
    /** @brief Magic number (SPLINTER_MAGIC) to verify integrity. */
    uint32_t magic;
    /** @brief Data layout version (SPLINTER_VER). */
    uint32_t version;
    /** @brief Total number of available key-value slots. */
    uint32_t slots;
    /** @brief Maximum size for any single value. */
    uint32_t max_val_sz;
    /** @brief Global epoch, incremented on any write. Used for change detection. */
    atomic_uint_least64_t epoch;
    /** @brief toggle for zeroing out the value region prior to writing there. */
    atomic_uint_least32_t auto_vacuum;

    /* Diagnostics: counts of parse failures reported by clients / harnesses */
    atomic_uint_least64_t parse_failures;
    atomic_uint_least64_t last_failure_epoch;
};
```

This is later type-defined as `splinter_header_t` opqauely in the library.

The only _current_ feature flag is `auto_vacuum`, which consumes the entire `atomic_uint_least32_t`. This isn't wasteful because we have no other features; there's no other 
purpose it can currently serve. 

Should that change, we can easily just break that down into 32 individual flags, with the first few being reserved for system use and the rest being available as `USR_FLAG_NN`.  The bus structure itself doesn't change (well, the variable name changes from `auto_vacuum` to `flags`), and 
then instead of treating it like a Boolean value, we do something like:

```c
/* System flags (0–15) */
#define SPL_FLAG_AUTO_VACUUM   (1u << 0)
#define SPL_FLAG_RES_GP2       (1u << 1)
#define SPL_FLAG_RES_GP3       (1u << 2)
/* ... */
#define SPL_FLAG_RES_GP16      (1u << 15)

/* User flags (16–31) */
#define SPL_FLAG_USR1          (1u << 16)
#define SPL_FLAG_USR2          (1u << 17)
/* ... */
#define SPL_FLAG_USR16         (1u << 31)
```

But we don't want to have to expose (in addition to these new constants) anything 
more than `splinter_header_t` in the public header. So, rather than just inviting
callers to operate on `flags` directly by exposing the structure, we create some
helpers, for instance:

```c
static inline void splinter_flag_set(splinter_bus_header_t *hdr, uint32_t mask) {
    atomic_fetch_or(&hdr->flags, mask);
}

static inline void splinter_flag_clear(splinter_bus_header_t *hdr, uint32_t mask) {
    atomic_fetch_and(&hdr->flags, ~mask);
}

static inline int splinter_flag_test(const splinter_bus_header_t *hdr, uint32_t mask) {
    return (atomic_load(&hdr->flags) & mask) != 0;
}

static inline uint32_t splinter_flag_snapshot(const splinter_bus_header_t *hdr) {
    return atomic_load(&hdr->flags);
}
```

This just splits use of them down the middle which isn't strictly necessary; I cant' imagine ever needing more than a few flags ever for internal housekeeping use, so the majority (28 - 30) of the rest of them could be implementation-defined pretty safely. 

Either way, you don't need to do anything other than set it to 0 before changing the header over, if you're currently using persistent mode. 

***If it looks like Splinter needs more than one feature flag***, this is definitely how I'll implement it. I'm just not doing it yet as it adds weight to the public header without more than one consumer of the field. 

