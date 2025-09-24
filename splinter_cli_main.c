#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "splinter_cli.h"

enum mode {
    MODE_REPL,
    MODE_NO_REPL
};

// Safely set mode from invoked name
// See POSIX exec family standards regarding argv[0] not being guaranteed,
// so we're extremely defensive about allowing the 'splinterctl' shortcut 
// to the --no-repl / --non-interactive modes.  
enum mode select_mode(const char *argv0) {
    char tmp[256];
    const char *prog = NULL;

    if (!argv0) return MODE_REPL;
    
    strncpy(tmp, argv0, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    prog = basename(tmp);

    if (strcmp(prog, "splinterctl") == 0) {
        return MODE_NO_REPL;
    } else if (strcmp(prog, "splinter_cli") == 0) {
        return MODE_REPL;
    }

    return MODE_REPL;
}

int main (int argc, char *argv[]) {
    enum mode m = MODE_REPL;

    // If you absolutely need to disable implicit checking based on
    // invocation, comment out the select_mode() line and just let 
    // arguments decide it.
    m = select_mode(argv[0]);
    
    puts("Hello from splinter_cli.");
    
    printf("Argument 0 is '%s', and %d arguments were passed.\n", argv[0], argc);
    printf("I am in %s mode.\n", 
        m == MODE_REPL ? "interactive" : "non-interactive");
    
    return 0;
}