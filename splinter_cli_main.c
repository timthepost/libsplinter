#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <limits.h>

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
        0,
        "help",
        4,
        "Help with this program, and commands it offers",
        -1,
        &cmd_help,
	    &help_cmd_help,
    },
    
    // more commands will go here ...
    
    { 0, NULL, 0, NULL, -1,  NULL , NULL }
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
    int rc = 0, _argc = 0, i;
    char *progname = basename(argv[0]), *buff;
    char prompt[64] = { 0 };
    char **mod_args = { 0 };

    // Malicious code can mess with numeric env values and try to make you overflow
    // other things. So we extract a long safely from the env, then an int safely from
    // the long. 
    long int historyenv = 0;
    int historylen = -1;
    const char *historyfile = getenv("SPLINTER_HISTORY_FILE");
    const char *tmp = getenv("SPLINTER_HISTORY_LEN");
    
    if (tmp != NULL) historyenv = strtol(tmp, &buff, 10);
    if (historyenv <= INT_MAX) {
        historylen = historyenv;
    } else {
        fprintf(stderr, "Warning: SPLINTER_HISTORY_LEN env variable value (%ld) exceeds INT_MAX; ignoring.",
            historyenv);
    }


    // If you absolutely need to disable the implicit mode check based on invocation, 
    // comment out m = select_mode() and allow arguments to decide it exclusively.
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

    // input processors add to history, we have to manage the rest.
    if (historyfile != NULL && historylen > 0) linenoiseHistoryLoad(historyfile);
    
    snprintf(prompt, 3, "# ");
    print_version_info(progname);

    if (m == MODE_REPL) {        
        fprintf(stderr,"To quit, press ctrl-c or ctrl-d.\n");
        do {
            // unpack the arguments into an argv[] style array
            mod_args = cli_input_args(prompt, &_argc);
            
            // ctrl-c or ctrl-d
            if (mod_args == NULL) break;
            // user just pressed enter alone
            if (_argc == 0) continue;

            for (i = 0; i < _argc; i++) printf("[%d/%d]: %s\n", i, _argc, mod_args[i]);
            cli_free_argv(mod_args);
            mod_args = NULL;
            _argc = 0;
        } while (1);
    } else {
        
        // non-interactive mode is processed from command line args, not the line editor
        // I need to figure out how I'm going to do that, exactly. 
        // Either way, it also needs to get added to history.

        fprintf(stderr, "non-interactive mode not yet implemented.\n");
        rc = 254;
    }

    // we've at least tried to process something at this point, so save history.
    if (historyfile != NULL && historylen > 0) linenoiseHistorySave(historyfile);
    
    return rc;
}
