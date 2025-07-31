#ifndef SHMBUS_H
#define SHMBUS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create a new shared‑memory bus. Returns 0 on success. */
int shmbus_create(const char *name, size_t slots, size_t max_value_sz);

/* Open an existing bus created elsewhere. */
int shmbus_open(const char *name);

/* Close current mapping (does NOT destroy memfd). */
void shmbus_close(void);

/* Set key → value. Returns 0 or error (<0). */
int shmbus_set(const char *key, const void *val, size_t len);

/* Get value for key. buf may be NULL to query size. */
int shmbus_get(const char *key, void *buf, size_t buf_sz, size_t *out_sz);

/* List keys. out_keys is an array of char* with space for max_keys. */
int shmbus_list(char **out_keys, size_t max_keys, size_t *out_count);

/* Block until key is modified or timeout (ms). */
int shmbus_poll(const char *key, uint64_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* SHMBUS_H */
