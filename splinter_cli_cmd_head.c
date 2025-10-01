#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "head";

void help_cmd_head(unsigned int level) {
    (void) level;
    printf("%s displays the metadata of a key in the store\n", modname);
    printf("Usage: %s <key_name>\n", modname);
    return;
}

int cmd_head(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    
    return 0;
}