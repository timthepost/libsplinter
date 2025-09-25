#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "splinter_cli.h"


void help_cmd_help(unsigned int level) {
    printf("You requested help for help at level %u\n", level);
}

int cmd_help(int argc, char *argv[]) {
    printf("This is cmd_help's entry point. argc: %d, argv[0]: %s\n",
        argc,
        argv[0] == NULL ? "null" : argv[0]);
    return 0;
}