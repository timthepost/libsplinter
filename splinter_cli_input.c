/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_input.c
 * @brief Splinter CLI input routines
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "splinter_cli.h"
#include "linenoise.h"

// parse line input into an argv[] style array
char **cli_input_args(const char *prompt, int *argc) {
    char *line = NULL;
    char **argv = { 0 };
    
    line = linenoise(prompt);
    if (line == NULL) return NULL;
    
    linenoiseHistoryAdd(line);
    argv = cli_unroll_argv(line, argc);
    linenoiseFree(line);

    return(argv);
}

// coming shortly (to implement 'watch')
int cli_input_args_async(const char *prompt) {
    // what will likely happen here is we have a thread funneling multiple watches into a single "console"
    // key to watch, so the console only has to poll one key. But I'm still deciding on how to hoist it.
    printf("%s Async not yet working.\n", prompt);
    return -1;
}

