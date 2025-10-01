#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "hist";

void help_cmd_hist(unsigned int level) {
    (void) level;
    printf("%s shows and clears the command history.\n", modname);
    printf("Usage: %s [clear]\n", modname);
    return;
}

int cmd_hist(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    
    return 0;
}