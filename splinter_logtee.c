/**
 * Copyright 2025 Tim Post
 * A 'tee' inspired utility that taps a bus key, sets up a watch, 
 * and writes the update to stdout (to be redirected elsewhere).
 * A non-destructive drain, or part of some other automation, whatever
 * works!
 * License: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "splinter.h"
#include "config.h"

#define MAX_LEN 4096
#define TIMEOUT_MS 100


static void print_help(const char *program_name) {
    printf("Usage: %s [OPTIONS] [BUS] [KEY]\n", program_name);
    printf("\nTail messages from a splinter bus key.\n");
    printf("\nArguments:\n");
    printf("  BUS                    Bus name (default: splinter_debug)\n");
    printf("  KEY                    Key name (default: __debug)\n");
    printf("\nOptions:\n");
    printf("  -h, --help            Show this help message\n");
    printf("  -v, --version         Show version information\n");
}

static void print_version(void) {
    printf("splinter_logtee %s\n", SPLINTER_VERSION);
}

int main(int argc, char **argv) {
    const char *bus = "splinter_debug";
    const char *key = "__debug";
    char msg[MAX_LEN];
    size_t msg_sz = 0;
    
    static struct option long_options[] = {
        {"help",    no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "h?v", long_options, NULL)) != -1) {
        switch (c) {
            case 'h':
            case '?':
                print_help(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;
        }
    }
    
    /* Handle positional arguments */
    if (optind < argc) {
        bus = argv[optind];
        if (optind + 1 < argc) {
            key = argv[optind + 1];
        }
    }

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
