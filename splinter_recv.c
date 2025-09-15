/**
 * A very crude tool to block and wait for a key to update, then print it.
 * Runs forever by default, --oneshot will cause it to exit after one event
 * fires.
 *
 * This needs to be proper-ized (--help, --version, short options, etc)
 */
#include "splinter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#define DEFAULT_BUS "splinter_debug"
#define DEFAULT_KEY "__debug"
#define MAX_LEN 4096
#define TIMEOUT_MS 100

int main(int argc, char **argv) {
    int rc = 0, errlvl = 0;
    unsigned int run = 1, oneshot = 0;
    char msg[MAX_LEN];
    size_t msg_sz = 0;
    
    if ((argc == 2) && (argv[1] != NULL) && (! strncmp(argv[1], "--oneshot", 9))) {
        oneshot = 1;
        fprintf(stderr, "splinter-recv: will exit after 1 event (--oneshot specified)\n");
    }

    if (splinter_create_or_open(DEFAULT_BUS, 128, 1024) != 0) {
        fprintf(stderr, "splinter-recv: failed to open bus %s\n", DEFAULT_BUS);
        return 1;
    }

    printf("splinter-recv: listening to %s on %s ...\n", DEFAULT_KEY, DEFAULT_BUS);
    if (! oneshot) fprintf(stderr, "splinter-recv: use --oneshot if you ever wish to exit after a single event.\n");
    
    do {
        rc = splinter_poll(DEFAULT_KEY, TIMEOUT_MS);
        if (rc == 0) {
            // watch fired
            if (splinter_get(DEFAULT_KEY, msg, sizeof(msg), &msg_sz) != 0) {
                fprintf(stderr, "splinter-recv: failed to read data from %s (key %s)\n", 
                    DEFAULT_BUS, DEFAULT_KEY);
                splinter_close();
                run = 0;
                errlvl = 2;
            }
            printf("splinter-recv: %.*s\n", (int)msg_sz, msg);
            fflush(stdout);
            if (oneshot) run = 0;
        }
    } while (run);

    return errlvl;
}
