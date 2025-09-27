/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_help.c
 * @brief Splinter CLI "help" command implementation
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "splinter_cli.h"


void help_cmd_help(unsigned int level) {
    printf("You requested help for help at level %u\n", level);
}

int cmd_help(int argc, char *argv[]) {
    int idx;

    if (argc == 1 ) {
        cli_show_modules();
        puts("\nFor help on a particular module, type 'help <module>'");
        return 0;
    }

    // something else was given
    idx = cli_find_module(argv[1]);

    // 0 is a valid index (the 'help' command by default)
    if (idx >= 0) {
        cli_show_module_help(idx, 1);
        return 0;
    } else {
        fprintf(stderr, "Could not find help for '%s'\n", argv[1]);
        errno = EINVAL;
        return 1;
    }
}
