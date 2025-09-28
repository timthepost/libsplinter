/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_util.c
 * @brief Splinter CLI utilties for working with modules
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "splinter_cli.h"

/**
 * Returns the index of the module in the modules array given its 
 * name, or -1 if not found.
 */
int cli_find_module(const char *name) {
    unsigned int i;
    
    for (i = 0; command_modules[i].name != NULL; i++) {
        if (! strncmp(name, command_modules[i].name, command_modules[i].name_len))
            return command_modules[i].id;
    }

    errno = EINVAL;

    return -1;
}

/**
 * See if the module at index idx is an alias by returning the index
 * of the module its aliased to, or -1 if not an alias. 
 */
int cli_find_alias(int idx) {
    if (! command_modules[idx].name) {
        errno = EINVAL;
        return -1;
    }
    return command_modules[idx].alias_of;
}

/**
 * Run the module at the specified index by its defined entry point and 
 * proxy its return value. Sets errno if idx is invalid.
 */
int cli_run_module(int idx, int argc, char *argv[]) {
    if (! command_modules[idx].name || ! command_modules[idx].entry) {
        errno = EINVAL;
        return -1;
    }
    return command_modules[idx].entry(argc, argv);
}

/**
 * Show a modules help (to the specified level) by index.
 */
void cli_show_module_help(int idx, unsigned int level) {
    if (! command_modules[idx].name || ! command_modules[idx].help) {
        errno = EINVAL;
        return;
    }
    command_modules[idx].help(level);
    return;
}

/**
 * A simple way to list modules
 */
#define LIST_BAR "----------------------------------"
void cli_show_modules(void) {
    int i;

    fprintf(stdout, "%-33s | %-33s\n%s+%s\n",
        "Module", 
        "Description",
        LIST_BAR,
        LIST_BAR);
        
    for (i = 0; command_modules[i].name; i++) {
        fprintf(stdout, "%-33s | %-33s\n",
            command_modules[i].name,
            command_modules[i].description);
    }
    
    return;
}
