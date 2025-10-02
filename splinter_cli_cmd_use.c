/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_use.c
 * @brief Implements the CLI 'use' command.
 */

#include <stdio.h>
#include <string.h>

#include "splinter.h"
#include "splinter_cli.h"

static const char *modname = "use";

void help_cmd_use(unsigned int level) {
    printf("%s help level %u\n", modname, level);
    return;
}

int cmd_use(int argc, char *argv[]) {
    if (argc != 2) {
        help_cmd_use(1);
        return 1;
    }
    splinter_close();
    if (splinter_open(argv[1]) == 0) {
        thisuser.store_conn = true;
        fprintf(stderr, "%s: now connected to %s\n", modname, argv[1]);
        return 0;
    } else {
        thisuser.store_conn = false;
        fprintf(stderr, "%s: failed to connect to %s\n", modname, argv[1]);
        fprintf(stderr, "%s: now disconnected.\n", modname);
        return 1;
    }
    return 0;
}
