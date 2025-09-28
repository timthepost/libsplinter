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

static const char *modname = "help";

void help_cmd_help(unsigned int level) {
    printf("%s provides help for this CLI.\nUsage: help [ext] <module_name>\n",
        modname);
    printf("Example: '%s ext help' shows the extended help display for this command, 'help'.\n",
        modname);
    printf("         'help' lists available commands and how to show additional help.\n");
    if (level) {
        puts("\nNot all commands have extended help displays.");
        puts("Please report problems with help coverage on Github:");
        puts("https://github.com/timthepost/libsplinter");
    }
}

int cmd_help(int argc, char *argv[]) {
    int idx;
    unsigned int ext_mode = 0;

    if (argc == 1 ) {
        cli_show_modules();
        puts("\nFor help on a particular module, type 'help <module>'");
        puts("For extended help on a particular module, type 'help ext <module>'");
        return 0;
    }

    if (! strncmp(argv[1], "ext", 3) && argc >= 3) {
        ext_mode = 1;
        idx = cli_find_module(argv[2]);
    } else {
        idx = cli_find_module(argv[1]);
    }

    // 0 is a valid index (the 'help' command by default)
    if (idx >= 0) {
        cli_show_module_help(idx, ext_mode);
        return 0;
    } else {
        fprintf(stderr, "Could not find help for '%s'\n", argv[1]);
        errno = EINVAL;
        return 1;
    }
}
