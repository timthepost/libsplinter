/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_get.c
 * @brief Implements the 'get' CLI command.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "get";

void help_cmd_get(unsigned int level) {
    (void) level;

    printf("%s gets the value of a key in the store.\n", modname);
    printf("Usage: %s <key_name>\n", modname);
    return;
}

int cmd_get(int argc, char *argv[]) {
    char buf[4096] = { 0 };
    size_t received = 0;
    int rc = -1;

    if (argc != 2) {
        help_cmd_get(1);
        return -1;
    }
    
    rc = splinter_get(argv[1], buf, sizeof(buf), &received);
    if (rc != 0) {
        fprintf(stderr, "%s: unable to retrieve key '%s'\n", modname, argv[1]);
        return rc;
    }

    printf("%lu : %s\n", received, buf);
    puts("");

    return 0;
}
