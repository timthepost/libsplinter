#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "unset";

void help_cmd_unset(unsigned int level) {
    (void) level;
    printf("%s un-sets the value of a key in the store\n", modname);
    printf("Usage: %s <key_name>\n", modname);
    return;
}

int cmd_unset(int argc, char *argv[]) {
    size_t deleted = 0;
    if (argc != 2) {
        help_cmd_unset(1);
        return 1;
    }

    deleted = splinter_unset(argv[1]);
    if ((int) deleted < 0)
        return (int) deleted;
    
    printf("%lu bytes deleted.\n", deleted);

    return 0;
}