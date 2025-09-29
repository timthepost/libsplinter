#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
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
    printf("Usage: %s <key_name_to_watch>\n", modname);
    if (level) {
        puts("\nYou can also use the 'splinter_recv' program to poll in scripts.");
    }
    return;
}

int cmd_watch(int argc, char *argv[]) {
    char msg[4096];
    size_t msg_sz = 0;
    char c, *key;
    int rc = -1;

    if (argc == 2) {
        key = argv[1];
    } else {
        fprintf(stderr, "Usage: %s <key>\nTry 'help ext watch' for help.\n", modname);
        return -1;
    }

    // TODO: validate key prior to starting
    
    setup_terminal();
    printf("Press Ctrl-] To Stop ...\n");
    
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
            fwrite(msg, 1, msg_sz, stdout);
            fputc('\n', stdout);
            fflush(stdout);
        }
    }

    thisuser.abort = 0;
    restore_terminal();
    
    return 0;
}