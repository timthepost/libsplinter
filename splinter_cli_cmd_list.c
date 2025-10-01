#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "list";

void help_cmd_list(unsigned int level) {
    (void) level;
    printf("%s lists keys in the currently selected store.\n", modname);

    return;
}

int cmd_list(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    
    return 0;
}