#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "splinter_cli.h"
#include "splinter.h"

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
    (void) level;
    return;
}

int cmd_watch(int argc, char *argv[]) {
    (void) argv;
    (void) argc;

    char c;
    int counter = 0;

    setup_terminal();
    
    printf("Press Ctrl-] (ASCII 29) to stop...\n");
    
    while (! thisuser.abort) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == 29) {  // Ctrl-] is ASCII 29
                tcflush(STDERR_FILENO, TCIFLUSH);
                thisuser.abort = 1;
                break;
            }
        }
        printf("Iteration %d\n", counter++);
        usleep(1000000);
    }
    
    thisuser.abort = 0;
    
    restore_terminal();
    
    return 0;
}