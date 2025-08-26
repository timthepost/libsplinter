#include "splinter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_BUS "splinter_debug"
#define DEFAULT_KEY "__debug"
#define MAX_LEN 4096
#define TIMEOUT_MS 100

int main(void) {
    int rc = 0, errlvl = 0;
    // will be modified by a future signal handler
    volatile unsigned int run = 1;
    char msg[MAX_LEN];
    size_t msg_sz = 0;
    
    if (splinter_create_or_open(DEFAULT_BUS, 128, 1024) != 0) {
        fprintf(stderr, "splinter-recv: failed to open bus %s\n", DEFAULT_BUS);
        return 1;
    }

    printf("splinter-recv: listening to %s on %s ...\n", DEFAULT_KEY, DEFAULT_BUS);

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
        }
    } while (run);

    return errlvl;
}
