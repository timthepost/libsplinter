#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "splinter_cli.h"
#include "linenoise.h"

// fires whenever the user presses tab
static void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == '\0') return;

    switch (buf[0]) {
        case 'h':
            linenoiseAddCompletion(lc,"help");
            break;
        default:
            break;
    }

    return;
}

// callback that provides completion hints
static char *hints(const char *buf, int *color, int *bold) { 
    /* foreground colors you can use:
     * red = 31
     * green = 32
     * yellow = 33
     * blue = 34
     * magenta = 35
     * cyan = 36
     * white = 37;
     */

    if (!strncasecmp(buf,"h", 4)) {
        *color = 32;
        *bold = 1;
        return "elp ";
    }

    return NULL;
}

// parse line input into an argv[] style array
char **cli_input_args(const char *prompt, int *argc) {
    char *line = NULL;
    char **argv = { 0 };
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    line = linenoise(prompt);
    if (line == NULL) return NULL;

    argv = cli_unroll_argv(line, argc);
    free(line);
    return(argv);
}

// coming shortly (to implement 'watch')
int cli_input_args_async(const char *prompt) {
    // what will likely happen here is we have a thread funneling multiple watches into a single "console"
    // key to watch, so the console only has to poll one key. But I'm still deciding on how to hoist it.
    printf("%s Async not yet working.\n", prompt);
    return -1;
}

