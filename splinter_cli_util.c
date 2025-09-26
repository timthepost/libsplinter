#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "splinter_cli.h"

/**
 * Returns the index of the module in the modules array given its 
 * name, or -1 if not found.
 */
int cli_find_module(const char *name) {
    printf("Looking for %s\n", name);
    return -1;
}

/**
 * See if the module at index idx is an alias by returning the index
 * of the module its aliased to, or -1 if not an alias. 
 */
int cli_find_alias(int idx) {
    printf("Looking for %d\n", idx);
    return -1;
}

/**
 * Run the module at the specified index by its defined entry point and 
 * proxy its return value. Sets errno if idx is invalid.
 */
int cli_run_module(int idx) {
    printf("Looking for %d\n", idx);
    errno = EINVAL;
    return -1;
}

/**
 * Show a modules help (to the specified level) by index.
 */
void cli_show_module_help(int idx) {
    printf("Looking for %u\n", idx);
    return;
}