#include "splinter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RUNA_KEY "runa_response"
#define MAX_LEN 4096
#define TIMEOUT_MS 100

#define BUS_NAME "/runa_debug"

int main(void) {
    if (splinter_open(BUS_NAME) != 0) {
        fprintf(stderr, "splinter-recv: failed to open bus\n");
        return 1;
    }

    // Poll until a message is available
    while (splinter_poll(RUNA_KEY, TIMEOUT_MS) != 0) {
        usleep(1000); // sleep 1ms
    }

    char msg[MAX_LEN];
    size_t msg_sz = 0;

    if (splinter_get(RUNA_KEY, msg, sizeof(msg), &msg_sz) != 0) {
        fprintf(stderr, "splinter-recv: failed to get message\n");
        splinter_close();
        return 2;
    }

    printf("%.*s\n", (int)msg_sz, msg);
    splinter_close();
    return 0;
}
