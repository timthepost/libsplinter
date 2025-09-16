/**
 * Copyright 2025 Tim Post
 * A tool to write to a key (for whatever use)
 * License: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "splinter.h"
#include "config.h"

static void print_help(const char *program_name) {
    printf("Usage: %s [OPTIONS] [BUS] [KEY] VALUE\n", program_name);
    printf("       %s [OPTIONS] [KEY] VALUE\n", program_name);
    printf("       %s [OPTIONS] VALUE\n", program_name);
    printf("\nSend a message to a splinter bus key.\n");
    printf("\nArguments:\n");
    printf("  BUS                    Bus name (default: %s)\n", DEFAULT_BUS);
    printf("  KEY                    Key name (default: %s)\n", DEFAULT_KEY);
    printf("  VALUE                  Message to send\n");
    printf("\nOptions:\n");
    printf("  -h, --help            Show this help message\n");
    printf("  -v, --version         Show version information\n");
}

static void print_version(void) {
    printf("splinter-send %s\n", SPLINTER_VERSION);
}

int main(int argc, char **argv) {
    const char *bus = DEFAULT_BUS;
    const char *key = DEFAULT_KEY;
    const char *value = NULL;
    
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
    int remaining_args = argc - optind;
    
    if (remaining_args < 1 || remaining_args > 3) {
        fprintf(stderr, "splinter-send: invalid number of arguments\n");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    if (remaining_args == 1) {
        value = argv[optind];
    } else if (remaining_args == 2) {
        key = argv[optind];
        value = argv[optind + 1];
    } else if (remaining_args == 3) {
        bus = argv[optind];
        key = argv[optind + 1];
        value = argv[optind + 2];
    }

    if (splinter_open_or_create(bus, 128, 1024) != 0) {
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
