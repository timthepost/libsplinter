/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_cmd_watch.c
 * @brief Implements the CLI 'watch' command.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "splinter_cli.h"
#include "splinter.h"

static const char *modname = "watch";

// make terminal non-blocking
void setup_terminal(void) {
    struct termios temp_tio;
    
    temp_tio = thisuser.term;
    
    // Disable canonical mode and echo
    temp_tio.c_lflag &= ~(ICANON | ECHO);
    temp_tio.c_cc[VMIN] = 0;   // Non-blocking read
    temp_tio.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &temp_tio);
    
    // Make stdin non-blocking
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    return;
}

// restore terminal to (whatever it was before)
void restore_terminal(void) {
    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &thisuser.term);
    
    // Make stdin blocking again
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    
    // Flush any pending input
    tcflush(STDIN_FILENO, TCIFLUSH);
}

void help_cmd_watch(unsigned int level) {
    printf("%s watches a key in the current store for changes.\n", modname);
    printf("Usage: %s <key_name_to_watch> [--oneshot]\n", modname);
    puts("If --oneshot is specified, watch will exit after one event.");
    if (level) {
        puts("\nYou can also use the 'splinter_recv' program to poll in scripts.");
    }
    return;
}

int cmd_watch(int argc, char *argv[]) {
    char msg[4096];
    size_t msg_sz = 0;

    char c, key[SPLINTER_KEY_MAX] = { 0 };
    int rc = -1;
    unsigned int oneshot = 0;

    if (argc >= 2)
        snprintf(key, sizeof(key) -1, "%s%s", getenv("SPLINTER_NS_PREFIX"), argv[1]);

    if (! key[0]) {
        fprintf(stderr, "Usage: %s <key> [--oneshot]\nTry 'help ext watch' for help.\n", modname);
        return -1;
    }

    if (argc == 3 ) {
        if (!strncasecmp(argv[2], "--oneshot", 10)) {
            oneshot = 1;
        } 
    }

    setup_terminal();

    if (! oneshot)
        puts("Press Ctrl-] To Stop ...");
    
    while (! thisuser.abort) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == 29) {  // Ctrl-] is ASCII 29
                tcflush(STDERR_FILENO, TCIFLUSH);
                thisuser.abort = 1;
                break;
            }
        }
        
        rc = splinter_poll(key, 100);
        if (rc == -1) {
            fprintf(stderr, "%s: invalid key: '%s'\n", modname, key);
            restore_terminal();
            return -1;
        }

        if (rc == 0) {
            if (splinter_get(key, msg, sizeof(msg), &msg_sz) != 0) {
                fprintf(stderr,
                        "splinter_logtee: failed to read key %s after update.\n",
                        key);
                return -1;
            }
            fprintf(stdout, "%lu:", msg_sz);
            fwrite(msg, 1, msg_sz, stdout);
            fputc('\n', stdout);
            fflush(stdout);
            if (oneshot) {
                // just raise it on behalf of the user since they specified it
                thisuser.abort = 1;
            }
        }
    }

    puts(""); // GET ends with one blank line, so we emulate that here as well.
    thisuser.abort = 0;
    restore_terminal();
    
    return 0;
}
