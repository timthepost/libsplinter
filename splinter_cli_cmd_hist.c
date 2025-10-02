/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_hist.c
 * @brief Implements the CLI 'hist' command.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "splinter_cli.h"
#include "3rdparty/grawk.h"
#include "3rdparty/linenoise.h"

static const char *modname = "hist";
extern char **history;
extern void freeHistory(void);

void help_cmd_hist(unsigned int level) {
    (void) level;
    printf("%s shows (filtered by [pattern]) the command history.\n", modname);
    printf("Usage: %s [pattern]\n", modname);
    return;
}

int cmd_hist(int argc, char *argv[]) {
    grawk_t *g = NULL;
    awk_pat_t *filter = NULL;
    int i;
    grawk_opts_t opts = { 
        .ignore_case = 0,
        .invert_match = 0,
        .quiet = 1
    };

    if (argc > 2) {
        help_cmd_hist(1);
        return -1;
    }
    
    g = grawk_init();
    if (g == NULL) {
        fprintf(stderr, "%s: unable to allocate memory to filter entries.\n", modname);
        errno = ENOMEM;
        return -1;
    }
    grawk_set_options(g, &opts);

    if (argc == 2) {
        filter = grawk_build_pattern(argv[1]);
        grawk_set_pattern(g, filter);
    }

    for (i = 0; history[i]; i++) {
        if (filter != NULL) {
            if (grawk_match(g, history[i])) {
                printf("%-4d: %s\n", i+1, history[i]);
            }
        } else {
            printf("%-4d: %s\n", i+1, history[i]);
        }
    }
    puts("");

    if (g != NULL)
        grawk_free(g);

    return 0;
}
