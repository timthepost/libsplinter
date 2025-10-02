/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_clear.c
 * @brief Implements the 'clear' CLI command.
 */

#include <stdio.h>
#include <string.h>
#include "splinter_cli.h"
#include "3rdparty/linenoise.h"

static const char *modname = "clear";

void help_cmd_clear(unsigned int level) {
    (void) level;
    printf("The '%s' command clears the screen.\n", modname);
    return;
}

int cmd_clear(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    
    linenoiseClearScreen();
    return 0;
}
