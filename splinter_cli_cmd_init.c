/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_init.c
 * @brief Implements the CLI 'init' command.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "splinter.h"
#include "splinter_cli.h"

static const char *modname = "init";
static const unsigned long max_slots = DEFAULT_SLOTS;
static const unsigned long max_val_len = DEFAULT_VAL_MAXLEN;

void help_cmd_init(unsigned int level) {
    (void) level;

    printf("%s creates a store to a default or specified geometry.\n", modname);
    printf("Usage: %s [store_name] [slots] [max_val_len]\n", modname);
    printf("If arguments are omitted, default values:\n\t%s -> (%lu x %lu)\n are used.\n",
        DEFAULT_BUS,
        max_slots, 
        max_val_len);

    return;
}

int cmd_init(int argc, char *argv[]) {
    char *buff = NULL, save[64] = { 0 };
    int rc = 0;
    unsigned int prev_conn = 0;

    if (thisuser.store_conn) {
        strncpy(save, thisuser.store, 64);
        prev_conn = 1;
    }

    switch (argc) {
        case 1:
            printf("Creating default store '%s' with  default geometry (%lu x %lu).\n",
                DEFAULT_BUS,
                max_slots,
                max_val_len);
            rc = splinter_create(DEFAULT_BUS, max_slots, max_val_len);
            break;
        case 2:
            printf("Creating '%s' with default geometry.\n", argv[1]);
            rc = splinter_create(argv[1], max_slots,  max_val_len);
            break;
        case 3:
            printf("Creating '%s' with %lu slots and default value length of %lu.\n", 
                argv[1], strtol(argv[2], &buff, 10),
                max_val_len);
            rc = splinter_create(argv[1], strtol(argv[2], &buff, 10), max_val_len);
            break;
        case 4:
            printf("Creating '%s' with %lu slots with a max value length of %lu.\n", 
                argv[1], 
                strtol(argv[2], &buff, 10),
                strtol(argv[3], &buff, 10)
            );
            rc = splinter_create(
                argv[1],
                strtol(argv[2], &buff, 10),
                strtol(argv[3], &buff, 10)
            );
            break;
        default:
            fprintf(stderr, "Unexpected number of arguments.\n");
            rc = 127;
    }

    if (rc < 0) {
        perror("splinter_create");
        goto restore_conn;
    }

    splinter_close();
    goto restore_conn;

restore_conn:
    if (prev_conn) {
        rc = splinter_open(save);
        if (rc != 0) {
            perror("splinter_open");
            fprintf(stderr, "warning: could not re-attach to %s, did something else remove it?",
            save);
            thisuser.store_conn = 0;
            return rc;
        } else {
            strncpy(thisuser.store, save, 64);
            thisuser.store_conn = 1;
        }
    }

    return rc;
}
