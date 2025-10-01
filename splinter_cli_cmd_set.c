#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "set";

void help_cmd_set(unsigned int level) {
   
    printf("%s sets the value of a key in the store\n", modname);
    printf("Usage: %s <key_name> \"<value>\"\n", modname);
    if (level)
        puts("\nKeys without spaces do not need to be quoted.");
    return;
}

int cmd_set(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    
    return 0;
}