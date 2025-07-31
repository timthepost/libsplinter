#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "shmbus.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#define SHMBUS_MAGIC 0x53484d42
#define SHMBUS_VER   1
#define KEY_MAX      64
#define NS_PER_MS    1000000ULL

struct shmbus_header {
    uint32_t magic;
    uint32_t version;
    uint32_t slots;
    uint32_t max_val_sz;
    atomic_uint_least64_t epoch;
};

struct shmbus_slot {
    atomic_uint_least64_t hash;
    atomic_uint_least64_t epoch;
    uint32_t val_off;
    uint32_t val_len;
    char key[KEY_MAX];
};

static void *g_base = NULL;
static size_t g_total_sz = 0;
static struct shmbus_header *H;
static struct shmbus_slot *S;
static char *VALUES;

static uint64_t fnv1a(const char *s) {
    uint64_t h = 14695981039346656037ULL;
    for (; *s; ++s) h = (h ^ (unsigned char)*s) * 1099511628211ULL;
    return h;
}

static inline size_t slot_idx(uint64_t hash, uint32_t slots) {
    return (size_t)(hash % slots);
}

static void add_ms(struct timespec *ts, uint64_t ms) {
    ts->tv_nsec += (ms % 1000) * NS_PER_MS;
    ts->tv_sec  += ms / 1000;
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec  += 1;
    }
}

static int map_fd(int fd, size_t size) {
    g_total_sz = size;
    g_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (g_base == MAP_FAILED) return -1;
    H = (struct shmbus_header *)g_base;
    S = (struct shmbus_slot *)(H + 1);
    VALUES = (char *)(S + H->slots);
    return 0;
}

int shmbus_create(const char *name, size_t slots, size_t max_value_sz) {
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd < 0) return -1;
    size_t region_sz = slots * max_value_sz;
    size_t total_sz  = sizeof(struct shmbus_header) + slots * sizeof(struct shmbus_slot) + region_sz;
    if (ftruncate(fd, (off_t)total_sz) != 0) return -1;
    if (map_fd(fd, total_sz) != 0) return -1;
    H->magic = SHMBUS_MAGIC;
    H->version = SHMBUS_VER;
    H->slots = (uint32_t)slots;
    H->max_val_sz = (uint32_t)max_value_sz;
    atomic_store(&H->epoch, 1);
    for (size_t i = 0; i < slots; ++i) {
        atomic_store(&S[i].hash, 0);
        atomic_store(&S[i].epoch, 0);
        S[i].val_off = (uint32_t)(i * max_value_sz);
        S[i].val_len = 0;
        memset(S[i].key, 0, KEY_MAX);
    }
    return 0;
}

int shmbus_open(const char *name) {
    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) return -1;
    struct stat st;
    if (fstat(fd, &st) != 0) return -1;
    if (map_fd(fd, (size_t)st.st_size) != 0) return -1;
    if (H->magic != SHMBUS_MAGIC || H->version != SHMBUS_VER) return -1;
    return 0;
}

void shmbus_close(void) {
    if (g_base) munmap(g_base, g_total_sz);
    g_base = NULL; H = NULL; S = NULL; VALUES = NULL; g_total_sz = 0;
}

int shmbus_set(const char *key, const void *val, size_t len) {
    if (!H || !key || len > H->max_val_sz) return -1;
    uint64_t h = fnv1a(key);
    size_t idx = slot_idx(h, H->slots);
    for (size_t i = 0; i < H->slots; ++i) {
        struct shmbus_slot *slot = &S[(idx + i) % H->slots];
        uint64_t slot_hash = atomic_load(&slot->hash);
        if (slot_hash == 0 || (slot_hash == h && strncmp(slot->key, key, KEY_MAX) == 0)) {
            if (val && len > 0) memcpy(VALUES + slot->val_off, val, len);
            slot->val_len = (uint32_t)len;
            strncpy(slot->key, key, KEY_MAX - 1);
            slot->key[KEY_MAX - 1] = '\0';
            atomic_store(&slot->hash, h);
            atomic_fetch_add(&slot->epoch, 1);
            atomic_fetch_add(&H->epoch, 1);
            return 0;
        }
    }
    return -1;
}

int shmbus_get(const char *key, void *buf, size_t buf_sz, size_t *out_sz) {
    if (!H || !key) return -1;
    uint64_t h = fnv1a(key);
    size_t idx = slot_idx(h, H->slots);
    for (size_t i = 0; i < H->slots; ++i) {
        struct shmbus_slot *slot = &S[(idx + i) % H->slots];
        if (atomic_load(&slot->hash) == h && strncmp(slot->key, key, KEY_MAX) == 0) {
            size_t len = slot->val_len;
            if (out_sz) *out_sz = len;
            if (buf) {
                if (buf_sz < len) { errno = EMSGSIZE; return -1; }
                memcpy(buf, VALUES + slot->val_off, len);
            }
            return 0;
        }
    }
    return -1;
}

int shmbus_list(char **out_keys, size_t max_keys, size_t *out_count) {
    if (!H || !out_keys || !out_count) return -1;
    size_t count = 0;
    for (size_t i = 0; i < H->slots && count < max_keys; ++i) {
        if (atomic_load(&S[i].hash) && S[i].val_len > 0) {
            out_keys[count++] = S[i].key;
        }
    }
    *out_count = count;
    return 0;
}

int shmbus_poll(const char *key, uint64_t timeout_ms) {
    if (!H || !key) return -1;
    uint64_t h = fnv1a(key);
    size_t idx = slot_idx(h, H->slots);
    struct shmbus_slot *slot = NULL;
    for (size_t i = 0; i < H->slots; ++i) {
        struct shmbus_slot *s = &S[(idx + i) % H->slots];
        if (atomic_load(&s->hash) == h && strncmp(s->key, key, KEY_MAX) == 0) {
            slot = s;
            break;
        }
    }
    if (!slot) return -1;
    uint64_t start_epoch = atomic_load(&slot->epoch);
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    add_ms(&deadline, timeout_ms);
    struct timespec sleep_ts = {0, 1000 * NS_PER_MS};
    while (atomic_load(&slot->epoch) == start_epoch) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        if ((now.tv_sec > deadline.tv_sec) ||
            (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec)) {
            errno = ETIMEDOUT;
            return -1;
        }
        nanosleep(&sleep_ts, NULL);
    }
    return 0;
}
