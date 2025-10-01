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
    (void) argc;
    (void) argv;
    
    return 0;
}