#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "export";

void help_cmd_export(unsigned int level) {
    (void) level;
    printf("%s exports all metadata, keys and values in a store.\n", modname);
    printf("Usage: %s <format> <location>\n", modname);
    return;
}

int cmd_export(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    
    return 0;
}