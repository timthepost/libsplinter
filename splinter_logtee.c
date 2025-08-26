#include "splinter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LEN 4096
#define TIMEOUT_MS 100

int main(int argc, char **argv) {
    const char *bus = (argc > 1) ? argv[1] : "splinter_debug";
    const char *key = (argc > 2) ? argv[2] : "__debug";

    char msg[MAX_LEN];
    size_t msg_sz = 0;

    if (splinter_create_or_open(bus, 128, 1024) != 0) {
        fprintf(stderr, "splinter_logtee: failed to open bus %s\n", bus);
        return 1;
    }

    while (1) {
        int rc = splinter_poll(key, TIMEOUT_MS);
        if (rc == 0) {
            if (splinter_get(key, msg, sizeof(msg), &msg_sz) != 0) {
                fprintf(stderr,
                        "splinter_logtee: failed to read from %s (key %s)\n",
                        bus, key);
                return 2;
            }
            fwrite(msg, 1, msg_sz, stdout);
            fputc('\n', stdout);
            fflush(stdout);
        }
    }
    return 0;
}
