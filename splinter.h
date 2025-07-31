// splinter.h
#ifndef SPLINTER_H
#define SPLINTER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int splinter_create(const char *name_or_path, size_t slots, size_t max_value_sz);
int splinter_open(const char *name_or_path);
int splinter_open_or_create(const char *name_or_path, size_t slots, size_t max_value_sz);
int splinter_create_or_open(const char *name_or_path, size_t slots, size_t max_value_sz);
void splinter_close(void);

int splinter_set(const char *key, const void *val, size_t len);
int splinter_get(const char *key, void *buf, size_t buf_sz, size_t *out_sz);
int splinter_list(char **out_keys, size_t max_keys, size_t *out_count);
int splinter_poll(const char *key, uint64_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // SPLINTER_H
