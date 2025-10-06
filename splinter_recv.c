/**
 * Copyright 2025 Tim Post
 * A tool to block and wait for a key to update, then print it.
 * Runs forever by default, --oneshot will cause it to exit after one event
 * fires.
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
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nListen for messages on a splinter bus key.\n");
    printf("\nOptions:\n");
    printf("  -o, --oneshot         Exit after receiving one message\n");
    printf("  -h, --help            Show this help message\n");
    printf("  -v, --version         Show version information\n");
}

static void print_version(void) {
    printf("splinter-recv %s\n", SPLINTER_VERSION);
}

int main(int argc, char **argv) {
    int rc = 0, errlvl = 0;
    unsigned int run = 1, oneshot = 0;
    char msg[MAX_LEN];
    size_t msg_sz = 0;
    
    static struct option long_options[] = {
        {"oneshot", no_argument, 0, 'o'},
        {"help",    no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "oh?v", long_options, NULL)) != -1) {
        switch (c) {
            case 'o':
                oneshot = 1;
                break;
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
    
    if (oneshot) {
        fprintf(stderr, "splinter-recv: will exit after 1 event (--oneshot specified)\n");
    }

    if (splinter_open_or_create(DEFAULT_BUS, 128, 1024) != 0) {
        fprintf(stderr, "splinter-recv: failed to open bus %s\n", DEFAULT_BUS);
        return 1;
    }

    printf("splinter-recv: listening to %s on %s ...\n", DEFAULT_KEY, DEFAULT_BUS);
    if (!oneshot) {
        fprintf(stderr, "splinter-recv: use --oneshot if you ever wish to exit after a single event.\n");
    }
    
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
