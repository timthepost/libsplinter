/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_tok.c
 * @brief Splinter CLI input tokenizer
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "splinter_cli.h"

/**
 * Dealing with potentially-quoted strings in C is always a pain. 
 * 
 * strtok() clobbers strings in-place, which is .. not ideal with dealing with
 * potentially quoted SET commands, etc. So, a custom "unrolling" helper
 * that deals with simple quotes.
 * 
 * This is meant to deal with things like:
 *     > set foo_key "fooo \"bar baz boo\" bar foo foobar"
 * It does NOT deal with single or nested quotes. 
 */
char **cli_unroll_argv(const char *input, int *out_argc) {
    int cap = 8, argc = 0, i;
    char **argv;

    if (!input) return NULL;
    const char *p = input;
    
    argv = malloc(cap * sizeof(char*));
    if (!argv) return NULL;

    while (*p) {
        /* skip whitespace */
        while (isspace((unsigned char)*p)) ++p;
        if (!*p) break;

        /* decide whether token is quoted */
        char *token = NULL;
        size_t tlen = 0;
        if (*p == '"') {
            ++p; /* skip opening quote */
            const char *start = p;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p+1)) p += 2; /* allow escaped chars inside quotes */
                else ++p;
            }
            tlen = p - start;
            token = malloc(tlen + 1);
            if (!token) goto fail;
            /* simple copy handling backslash escapes for \" and \\ */
            const char *q = start;
            char *w = token;
            while (q < p) {
                // advance past escape
                if (*q == '\\' && (q + 1) < p) ++q;
                // now just a simple copy
                *w++ = *q++;
            }
            *w = '\0';
            if (*p == '"') ++p; /* skip closing quote */
        } else {
            const char *start = p;
            while (*p && !isspace((unsigned char)*p)) {
                ++p;
            }
            tlen = p - start;
            token = malloc(tlen + 1);
            if (!token) goto fail;
            memcpy(token, start, tlen);
            token[tlen] = '\0';
        }

        if (argc >= cap) {
            cap *= 2;
            char **tmp = realloc(argv, cap * sizeof(char*));
            if (!tmp) { free(token); goto fail; }
            argv = tmp;
        }
        argv[argc++] = token;
    }

    /* NULL-terminate */
    char **tmp = realloc(argv, (argc + 1) * sizeof(char*));
    if (!tmp) goto fail;
    argv = tmp;
    argv[argc] = NULL;
    *out_argc = argc;
    return argv;

fail:
    if (argv) {
        for (i = 0; i < argc; ++i) free(argv[i]);
        free(argv);
    }
    return NULL;
}

/**
 * Slice the last n elements of an array (usually args) into
 * a new array beginning at the given offset as [0].
 */
char **cli_slice_args(char *const src[], size_t n) {
    size_t i = 0, start = 0, len = 0;

    if (!src) return NULL;

    while (src[len] != NULL) ++len;

    /* clamp to available elements */
    if (n > len) n = len;

    char **dst = malloc((n + 1) * sizeof(char *));
    if (!dst) return NULL;

    start = len - n;
    for (i = 0; i < n; ++i) {
        dst[i] = src[start + i];
    }
    dst[n] = NULL;

    return dst;
}

/**
 * Helper to free an allocated argument stack
 */
void cli_free_argv(char *argv[]) {
    int i;

    if (!argv) return;
    for (i = 0; argv[i]; ++i) free(argv[i]);
    free(argv);
}
