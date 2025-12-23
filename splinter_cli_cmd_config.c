/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_config.c
 * @brief Implements the CLI 'config' command.
 */

#include <stdio.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "config";

void help_cmd_config(unsigned int level) {
    (void) level;
    printf("Usage: %s\n       %s [feature_flag] [flag_value]\n", modname, modname);
    printf("If no other arguments are given, %s displays the current bus settings.\n", modname);
    return;
}

static void show_bus_config(void) {
    splinter_header_snapshot_t snap = {0};

    splinter_get_header_snapshot(&snap);

    printf("magic:       %u\n", snap.magic);
    printf("version:     %u\n", snap.version);
    printf("slots:       %u\n", snap.slots);
    printf("max_val_sz:  %u\n", snap.max_val_sz);
    printf("epoch:       %lu\n", snap.epoch);
    printf("auto_vacuum: %u\n", snap.auto_vacuum);
    puts("");
    
    return;
}

int cmd_config(int argc, char *argv[]) {
    (void) argv;

    if (argc == 1) {
        show_bus_config();
        return 0;
    }

    help_cmd_config(1);
    
    return 1;
}
