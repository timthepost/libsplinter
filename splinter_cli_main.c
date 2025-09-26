#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "splinter_cli.h"
#include "config.h"
#include "linenoise.h"

/**
 * TODO:
 *  - Implement command structures, helpers, aliases and "watch"
 *  - Test async input with watch
 *  - Implement getopt_long() arguments
 *  - Implement commands and command help displays
 *     - first the structural ones (help, exit, etc)
 *     - then the rest of the splinter ones (get, set, etc)
 *     - then access them non-interactively successfully as appropriate
 *  - (Maybe) implement code for editor with "edit" command just to access for now?
 *  - (Maybe) implement some kind of test harness to test the commands?
 */

enum mode {
    MODE_REPL,
    MODE_NO_REPL
};

cli_module_t command_modules[] = {
    {
        "help",
        "Help with this program, and commands it offers",
        false,
        false,
        &cmd_help,
	    &help_cmd_help,
    },
    
    // more commands will go here ...
    
    { NULL , NULL , NULL , NULL,  NULL , NULL }
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

void print_version_info(char *progname) {
    fprintf(stderr,  "%s version %s build %s\n",
        progname, 
        SPLINTER_VERSION, 
        SPLINTER_BUILD);
    return;
}

void  print_usage(char *progname) {
    fprintf(stderr, "Usage: %s\n       %s  [--no-repl] [--help] [bus_name] [command1 command2 ...]\n",
        progname, 
        progname);
    return;
}

int main (int argc, char *argv[]) {
    enum mode m = MODE_REPL;
    int rc = 0;
    char *progname = basename(argv[0]);

    // If you absolutely need to disable implicit checking based on
    // invocation, comment out the select_mode() line and just let 
    // arguments decide it.
    m = select_mode(progname);

    /* Will probably use getopt_long() */
    while(argc > 1) {
        argc--;
        argv++;
        if (!strncmp(*argv,"--no-repl", 9)) {
            m = MODE_NO_REPL;
        } else if (!strncmp(*argv,"--help", 6)) {
            print_version_info(progname);
            print_usage(progname);
            // un-comment to trap and print key bindings
            // linenoisePrintKeyCodes();
            return 0;
        } else {
            fprintf(stderr,"%s: unsure how to handle argument %d: %s\n",
                progname, 
                argc, 
                *argv);
        }
    }

    print_version_info(progname);

    if (m == MODE_REPL) {        
        fprintf(stderr,"To quit press ctrl-c, ctrl-d or type 'quit'.\n");
        // read, hint, complete, tokenize, dispatch, repeat.
        rc = cli_handle_input(0, "> ");
    } else {
        fprintf(stderr, "non-interactive mode not yet implemented.\n");
        rc = 254;
    }

    return rc;
}
