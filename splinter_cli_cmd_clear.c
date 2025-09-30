#include <stdio.h>
#include <string.h>
#include "splinter_cli.h"
#include "3rdparty/linenoise.h"

static const char *modname = "clear";

void help_cmd_clear(unsigned int level) {

    printf("The '%s' command clears the screen.\n", modname);
    if (level > 0)
        printf("No further information is available about '%s'\n", 
            modname);
    return;
}

int cmd_clear(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    
    linenoiseClearScreen();
    return 0;
}