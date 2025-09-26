#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "splinter_cli.h"
#include "linenoise.h"

// Will need to manually (for now) curate commands here

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
    /* 
     * Colors:
     * red = 31
     * green = 32
     * yellow = 33
     * blue = 34
     * magenta = 35
     * cyan = 36
     * white = 37;
    */

    // strncasecmp len should be the length of the full command 
    // after completion, for completion to work correctly.

    if (!strncasecmp(buf,"h", 4)) {
        *color = 32;
        *bold = 1;
        return "elp ";
    }

    return NULL;
}

int cli_handle_input(int async, const char *prompt) {
    char *line = NULL;
    const char *histfile = "~/.splinter_history";
    int argc = 0, i;
    char **argv = NULL;

    // hints and completion are a little sketchy right now ...
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    // history naming after argv[0] (splinterctl separate) or is this okay?
    linenoiseHistoryLoad(histfile);

    do {
        if (!async) {
            line = linenoise(prompt);
            if (line == NULL) break;
        } else {
            fprintf(stderr, "Note: async behavior is not yet stable.\n");
            struct linenoiseState ls;
            char buf[1024];
            linenoiseEditStart(&ls,-1,-1,buf,sizeof(buf), prompt);
            while(1) {
                fd_set readfds;
                struct timeval tv;
                int retval;

                FD_ZERO(&readfds);
                FD_SET(ls.ifd, &readfds);
                tv.tv_sec = 1; // 1 sec timeout
                tv.tv_usec = 0;

                retval = select(ls.ifd+1, &readfds, NULL, NULL, &tv);
                if (retval == -1) {
                    perror("select()");
                    return -1;
                } else if (retval) {
                    line = linenoiseEditFeed(&ls);
                    if (line != linenoiseEditMore) break;
                } else {
                    // Timeout
                    static int counter = 0;
                    linenoiseHide(&ls);
                    printf("Async output %d.\n", counter++);
                            linenoiseShow(&ls);
                }
            }
            linenoiseEditStop(&ls);
            if (line == NULL) return 0; /* Ctrl+D/C. */
        }

        if (line[0] != '\0') {
            // TODO: See if it's a runnable command, if so grab args and execute
            // for now we just reply back.
            printf("echoreply: '%s'\n", line);
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(histfile);
            argv = cli_unroll_argv(line, &argc);
            for (i = 0; argv[i]; i++)
                fprintf(stdout, "  > %d / %d: %s\n", i, argc, argv[i]);
            cli_free_argv(argv);
        }

        free(line);

        // loop end
    } while (1);

    return 0;
}

