/**
 * MRSW tests for splinter (the ones we actually care about)
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include "splinter.h"

typedef struct {
    const char *store_name;
    int slots;
    int max_value_size;

    int num_threads;        // total threads (includes writer)
    int test_duration_ms;
    int num_keys;           // size of hot keyset
    int writer_period_us;   // sleep between writes (0 = as fast as possible)
} cfg_t;

typedef struct {
    atomic_int total_gets;
    atomic_int total_sets;
    atomic_int get_ok;
    atomic_int set_ok;
    atomic_int get_fail;
    atomic_int set_fail;
    atomic_int integrity_fail;  // value parse/monotonicity failures
    atomic_int retries;         // seqlock retries
} counters_t;

typedef struct {
    cfg_t *cfg;
    counters_t *ctr;
    volatile int *running;

    // shared keyset
    char **keys;
    int num_keys;
} shared_t;

static inline long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)(ts.tv_sec*1000LL + ts.tv_nsec/1000000LL);
}

/* --- Per-thread RNG (xorshift32) --- */
static inline unsigned xorshift32(unsigned *s) {
    unsigned x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

static void *writer_main(void *arg) {
    shared_t *sh = (shared_t*)arg;
    cfg_t *cfg = sh->cfg;

    char *buf = malloc(cfg->max_value_size);
    if (!buf) { perror("malloc"); return NULL; }

    unsigned ver = 1;
    size_t payload_len = cfg->max_value_size / 2;
    if (payload_len < 64) payload_len = 64;

    while (*sh->running) {
        for (int i = 0; i < sh->num_keys && *sh->running; i++) {
            unsigned long nonce = (unsigned long)now_ms();
            int n = snprintf(buf, cfg->max_value_size,
                             "ver:%u|nonce:%lu|data:", ver, nonce);
            if (n <= 0 || n >= cfg->max_value_size) {
                atomic_fetch_add(&sh->ctr->set_fail, 1);
                continue;
            }

            size_t remain = cfg->max_value_size - (size_t)n - 1;
            size_t fill = payload_len < remain ? payload_len : remain;
            memset(buf + n, 'A' + (ver % 26), fill);
            buf[n + fill] = '\0';
            size_t len = n + fill;

            int rc = splinter_set(sh->keys[i], buf, len);
            atomic_fetch_add(&sh->ctr->total_sets, 1);
            if (rc == 0) {
                atomic_fetch_add(&sh->ctr->set_ok, 1);
            } else {
                atomic_fetch_add(&sh->ctr->set_fail, 1);
            }
            if (cfg->writer_period_us > 0) usleep(cfg->writer_period_us);
        }
        ver++;
    }

    free(buf);
    return NULL;
}

static bool parse_ver(const char *val, size_t len, unsigned *out_ver) {
    if (len < 6) return false;
    if (memcmp(val, "ver:", 4) != 0) return false;
    char tmp[16] = {0};
    size_t i = 4, j = 0;
    while (i < len && j < sizeof(tmp)-1 && val[i] >= '0' && val[i] <= '9') {
        tmp[j++] = val[i++];
    }
    if (i >= len || val[i] != '|') return false;
    *out_ver = (unsigned)strtoul(tmp, NULL, 10);
    return true;
}

static void *reader_main(void *arg) {
    shared_t *sh = (shared_t*)arg;
    cfg_t *cfg = sh->cfg;

    char *buf = malloc((size_t)cfg->max_value_size + 1);
    if (!buf) { perror("malloc"); return NULL; }
    size_t got_size = 0;

    // Per-thread observed versions
    unsigned *observed = calloc((size_t)sh->num_keys, sizeof(unsigned));
    if (!observed) { perror("calloc"); free(buf); return NULL; }

    static __thread unsigned seed = 0;
    if (seed == 0)
        seed = 0x9e3779b9u ^ (unsigned)pthread_self();

    while (*sh->running) {
        for (int t = 0; t < 256 && *sh->running; t++) {
            int idx = (int)(xorshift32(&seed) % sh->num_keys);

            for (;;) {
                if (!*sh->running) break;

                int rc = splinter_get(sh->keys[idx], buf,
                                      (size_t)cfg->max_value_size, &got_size);
                atomic_fetch_add(&sh->ctr->total_gets, 1);

                if (rc == 0) {
                    atomic_fetch_add(&sh->ctr->get_ok, 1);

                    unsigned ver = 0;
                    if (got_size == 0 || !parse_ver(buf, got_size, &ver)) {
                        atomic_fetch_add(&sh->ctr->integrity_fail, 1);
                    } else {
                        unsigned prev = observed[idx];
                        if (ver < prev) {
                            atomic_fetch_add(&sh->ctr->integrity_fail, 1);
                        } else if (ver > prev) {
                            observed[idx] = ver;
                        }
                    }
                    break;
                } else if (errno == EAGAIN) {
                    atomic_fetch_add(&sh->ctr->retries, 1);
#if defined(__x86_64__) || defined(__i386__)
                    __asm__ __volatile__("pause" ::: "memory");
#endif
                    static __thread unsigned spin = 0;
                    if ((++spin & 0xFF) == 0) sched_yield();
                    continue;
                } else {
                    atomic_fetch_add(&sh->ctr->get_fail, 1);
                    break;
                }
            }

            if ((t & 63) == 0) sched_yield();
        }
    }

    free(observed);
    free(buf);
    return NULL;
}

static void prepopulate(shared_t *sh) {
    char v[128];
    for (int i = 0; i < sh->num_keys; i++) {
        int n = snprintf(v, sizeof(v), "ver:%u|nonce:%lu|data:SEED",
                         1u, (unsigned long)now_ms());
        if (n > 0) (void)splinter_set(sh->keys[i], v, (size_t)n);
    }
}

static void print_stats(cfg_t *cfg, counters_t *c, long ms) {
    int gets = atomic_load(&c->total_gets);
    int sets = atomic_load(&c->total_sets);
    int okg  = atomic_load(&c->get_ok);
    int oks  = atomic_load(&c->set_ok);
    int fget = atomic_load(&c->get_fail);
    int fset = atomic_load(&c->set_fail);
    int bad  = atomic_load(&c->integrity_fail);
    int retr = atomic_load(&c->retries);

    double sec = ms / 1000.0;
    double ops = (gets + sets) / sec;

    printf("\n===== MRSW STRESS RESULTS =====\n");
    printf("Threads: %d (readers=%d, writer=1)\n",
           cfg->num_threads, cfg->num_threads - 1);
    printf("Duration: %d ms\n", cfg->test_duration_ms);
    printf("Hot keys: %d\n", cfg->num_keys);
    printf("Total ops: %d (gets=%d, sets=%d)\n", gets + sets, gets, sets);
    printf("Throughput: %.0f ops/sec\n", ops);
    printf("Get: ok=%d fail=%d\n", okg, fget);
    printf("Set: ok=%d fail=%d\n", oks, fset);
    printf("Integrity failures: %d\n", bad);
    printf("Retries (EAGAIN):   %d (%.2f%% of gets, %.2f per successful get)\n",
           retr,
           gets ? (100.0 * retr / gets) : 0.0,
           okg  ? ((double)retr / okg)   : 0.0);
    printf("================================\n");
}

static void usage(const char *prog) {
    fprintf(stderr,
        "usage: %s [--threads N] [--duration-ms D] [--keys K] [--store NAME]\n"
        "          [--slots S] [--max-value B] [--writer-us U]\n", prog);
}

int main(int argc, char **argv) {
    cfg_t cfg = {
        .store_name = "mrsw_store",
        .slots = 50000,
        .max_value_size = 4096,
        .num_threads = 32,
        .test_duration_ms = 60000,
        .num_keys = 20000,
        .writer_period_us = 0,
    };

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--threads") && i+1 < argc) cfg.num_threads = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--duration-ms") && i+1 < argc) cfg.test_duration_ms = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--keys") && i+1 < argc) cfg.num_keys = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--store") && i+1 < argc) cfg.store_name = argv[++i];
        else if (!strcmp(argv[i], "--slots") && i+1 < argc) cfg.slots = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--max-value") && i+1 < argc) cfg.max_value_size = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--writer-us") && i+1 < argc) cfg.writer_period_us = atoi(argv[++i]);
        else { usage(argv[0]); return 2; }
    }
    if (cfg.num_threads < 2) cfg.num_threads = 2;

    if (splinter_create_or_open(cfg.store_name, cfg.slots, cfg.max_value_size) != 0) {
        perror("splinter_create_or_open");
        return 1;
    }

    printf("This is going to take a little while ...\n");

    char **keys = calloc((size_t)cfg.num_keys, sizeof(char*));
    if (!keys) { perror("calloc"); return 1; }

    for (int i = 0; i < cfg.num_keys; i++) {
        keys[i] = malloc(32);
        if (!keys[i]) { perror("malloc"); return 1; }
        snprintf(keys[i], 32, "k%08d", i);
    }

    counters_t ctr = {0};
    volatile int running = 1;
    shared_t sh = {
        .cfg = &cfg,
        .ctr = &ctr,
        .running = &running,
        .keys = keys,
        .num_keys = cfg.num_keys,
    };

    prepopulate(&sh);

    pthread_t *th = calloc((size_t)cfg.num_threads, sizeof(pthread_t));
    if (!th) { perror("calloc"); return 1; }

    if (pthread_create(&th[0], NULL, writer_main, &sh) != 0) {
        perror("pthread_create writer");
        return 1;
    }
    for (int i = 1; i < cfg.num_threads; i++) {
        if (pthread_create(&th[i], NULL, reader_main, &sh) != 0) {
            perror("pthread_create reader");
            running = 0;
            break;
        }
    }

    long start = now_ms();
    while (now_ms() - start < cfg.test_duration_ms) {
        usleep(10000);
    }
    running = 0;

    for (int i = 0; i < cfg.num_threads; i++) pthread_join(th[i], NULL);
    long elapsed = now_ms() - start;

    print_stats(&cfg, &ctr, elapsed);

    for (int i = 0; i < cfg.num_keys; i++) free(keys[i]);
    free(keys);
    free(th);

    splinter_close();
    return 0;
}
