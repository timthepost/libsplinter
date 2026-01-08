/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_head.c
 * @brief Implements the CLI 'head' command.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "splinter_cli.h"

static const char *modname = "head";

void help_cmd_head(unsigned int level) {
    (void) level;
    printf("%s displays the metadata of a key in the store\n", modname);
    printf("Usage: %s <key_name>\n", modname);
    return;
}

int cmd_head(int argc, char *argv[]) {
    char key[SPLINTER_KEY_MAX] = { 0 };

    if (argc != 2) {
        help_cmd_head(1);
        return -1;
    }

    snprintf(key, sizeof(key) - 1, "%s%s", getenv("SPLINTER_NS_PREFIX"), argv[1]);
    cli_show_key_config(key, modname);
    
    return 0;
}