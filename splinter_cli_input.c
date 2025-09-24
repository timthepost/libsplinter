#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "splinter_cli.h"
#include "linenoise.h"

void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == '\0') return;

    switch (buf[0]) {
        case 'f':
            linenoiseAddCompletion(lc,"fizz");
            break;
        case 'b':
            linenoiseAddCompletion(lc,"buzz");
            break;
        default:
            break;
    }

    return;
}

char *hints(const char *buf, int *color, int *bold) {
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

    // TODO - Pull these in from exported command modules

    if (!strncasecmp(buf,"fi", 4)) {
        *color = 35;
        *bold = 0;
        return "zz ";
    }

    if (!strncasecmp(buf,"bu", 4)) {
        *color = 36;
        *bold = 0;
        return "zz ";
    }

    return NULL;
}

int cli_handle_input(int async, const char *prompt) {
    char *line = NULL;
    const char *histfile = "~/.splinter_history";

    // hints and completion are a little sketchy right now ...
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);

    // history naming after argv[0] (splinterctl separate) or is this okay?
    linenoiseHistoryLoad(histfile);

    do {
        if (!async) {
            /**
             * This is the most common use.
             * If not async (e.g. not watching a watch fire while also interactive) 
             * then life is easy - just wait for CTRL-D/C and be done. 
             */
            line = linenoise(prompt);
            if (line == NULL) break;
        } else {
            /**
             * This is lesser-common, but also the most complex case, 
             * it's likely we're watching polls fire while also in interactive input
             * mode. Fortunately, line noise provides for this (e.g. IRC use)
             */
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
                    exit(1);
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
            if (line == NULL) exit(0); /* Ctrl+D/C. */
        }

        // TODO - Work in the exported command structure here
        // Demo from library to test integration:

        if (line[0] != '\0') {
            printf("echoreply: '%s'\n", line);
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(histfile);
        }

        free(line);
    } while (1);

    return 0;
}
