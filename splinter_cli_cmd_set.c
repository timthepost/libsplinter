#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "set";

// we need to get this dynamically from the bus
// also have this issue in get. I'm just using a 4k
// ceiling for now.
#define SET_CMD_MAX_LEN 4096

void help_cmd_set(unsigned int level) {
   
    printf("%s sets the value of a key in the store\n", modname);
    printf("Usage: %s <key_name> \"<value>\"\n", modname);
    if (level)
        puts("\nKeys without spaces do not need to be quoted.");
    return;
}

int cmd_set(int argc, char *argv[]) {
    if (argc < 3) {
        help_cmd_set(1);
        return 1;
    }

    return splinter_set(argv[1], argv[2], strnlen(argv[2], 4096));
}