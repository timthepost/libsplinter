/**
 * Copyright 2025 Tim Post
 * License: Apache 2
 * @file splinter.h
 * @brief Public API for the libsplinter shared memory key-value store.
 *
 * This header defines the public functions for creating, opening, interacting
 * with, and closing a splinter store.
 */
#ifndef SPLINTER_H
#define SPLINTER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates and initializes a new splinter store.
 * @param name_or_path The name of the shared memory object or path to the file.
 * @param slots The total number of key-value slots to allocate.
 * @param max_value_sz The maximum size in bytes for any single value.
 * @return 0 on success, -1 on failure (e.g., store already exists).
 */
int splinter_create(const char *name_or_path, size_t slots, size_t max_value_sz);

/**
 * @brief Opens an existing splinter store.
 * @param name_or_path The name of the shared memory object or path to the file.
 * @return 0 on success, -1 on failure (e.g., store does not exist).
 */
int splinter_open(const char *name_or_path);

/**
 * @brief Opens an existing splinter store, or creates it if it does not exist.
 * @param name_or_path The name of the shared memory object or path to the file.
 * @param slots The total number of key-value slots if creating.
 * @param max_value_sz The maximum value size in bytes if creating.
 * @return 0 on success, -1 on failure.
 */
int splinter_open_or_create(const char *name_or_path, size_t slots, size_t max_value_sz);

/**
 * @brief Creates a new splinter store, or opens it if it already exists.
 * @param name_or_path The name of the shared memory object or path to the file.
 * @param slots The total number of key-value slots if creating.
 * @param max_value_sz The maximum value size in bytes if creating.
 * @return 0 on success, -1 on failure.
 */
int splinter_create_or_open(const char *name_or_path, size_t slots, size_t max_value_sz);

/**
 * @brief Closes the splinter store and unmaps the shared memory region.
 */
void splinter_close(void);

/**
 * @brief Sets or updates a key-value pair in the store.
 * @param key The null-terminated key string.
 * @param val Pointer to the value data.
 * @param len The length of the value data. Must not exceed `max_val_sz`.
 * @return 0 on success, -1 on failure (e.g., store is full).
 */
int splinter_set(const char *key, const void *val, size_t len);

/**
 * @brief Retrieves the value associated with a key.
 * @param key The null-terminated key string.
 * @param buf The buffer to copy the value data into. Can be NULL to query size.
 * @param buf_sz The size of the provided buffer.
 * @param out_sz Pointer to a size_t to store the value's actual length. Can be NULL.
 * @return 0 on success, -1 on failure. If buf_sz is too small, sets errno to EMSGSIZE.
 */
int splinter_get(const char *key, void *buf, size_t buf_sz, size_t *out_sz);

/**
 * @brief Lists all keys currently in the store.
 * @param out_keys An array of `char*` to be filled with pointers to the keys.
 * @param max_keys The maximum number of keys to write to `out_keys`.
 * @param out_count Pointer to a size_t to store the number of keys found.
 * @return 0 on success, -1 on failure.
 */
int splinter_list(char **out_keys, size_t max_keys, size_t *out_count);

/**
 * @brief Waits for a key's value to be changed.
 * @param key The key to monitor for changes.
 * @param timeout_ms The maximum time to wait in milliseconds.
 * @return 0 if the value changed, -1 on timeout or if the key doesn't exist.
 */
int splinter_poll(const char *key, uint64_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // SPLINTER_H

