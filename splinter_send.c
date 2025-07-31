// cli/splinter_send.c
#include "splinter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUS "/runa_debug"
#define DEFAULT_KEY "runa_response"

int main(int argc, char **argv) {
    const char *bus = DEFAULT_BUS;
    const char *key = DEFAULT_KEY;
    const char *value;

    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: %s [bus_path] [key] value\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        value = argv[1];
    } else if (argc == 3) {
        key = argv[1];
        value = argv[2];
    } else {
        bus = argv[1];
        key = argv[2];
        value = argv[3];
    }

    if (splinter_open(bus) != 0) {
        fprintf(stderr, "splinter-send: failed to open bus at %s\n", bus);
        return 1;
    }

    if (splinter_set(key, value, strlen(value)) != 0) {
        fprintf(stderr, "splinter-send: failed to send value to key %s\n", key);
        return 2;
    }

    splinter_close();
    return 0;
}
