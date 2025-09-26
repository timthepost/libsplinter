/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_main.c
 * @brief Splinter CLI main entry / completion & hint setup / parsing / executing
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "splinter_cli.h"
#include "config.h"
#include "linenoise.h"


/**
 * We deviate from linenoise default here (which hides these)
 * but we need direct access to the history array for non-repl
 * cleanup on exit (non-repl gets logged in history, too, but 
 * different handlers when not getting direct input). 
 */
extern char **history;
extern void freeHistory(void);

/**
 * TODO:
 *  - Implement getopt_long() arguments
 *  - Implement commands and command help displays
 *     - first the structural ones (help, config, etc)
 *     - then the rest of the splinter ones (get, set, etc)
 *     - then access them non-interactively successfully as appropriate
 *  - Implement "watch"
 *  - Test async input with watch
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

    if (strncmp(prog, "splinterctl", 11) == 0) {
        return MODE_NO_REPL;
    } else if (strncmp(prog, "splinter_cli", 13) == 0) {
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

// fires whenever the user presses tab for tab / auto completion
static void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == '\0') return;

    switch (buf[0]) {
        case 'h':
            linenoiseAddCompletion(lc,"help");
            break;
        default:
            break;
    }

    return;
}

// callback that provides completion hints while typing
static char *hints(const char *buf, int *color, int *bold) { 
    /* foreground colors you can use:
     * red = 31
     * green = 32
     * yellow = 33
     * blue = 34
     * magenta = 35
     * cyan = 36
     * white = 37;
     */

    if (!strncasecmp(buf,"h", 4)) {
        *color = 32;
        *bold = 1;
        return "elp ";
    }

    return NULL;
}

int main (int argc, char *argv[]) {
    enum mode m = MODE_REPL;
    int rc = 0, _argc = 0, idx = -1;
    char *progname = basename(argv[0]), *buff;
    char prompt[64] = { 0 };
    char **mod_args = { 0 };

    // Malicious code can mess with numeric env values and try to make you overflow
    // other things. So we extract a long safely from the env, then an int safely from
    // the long. 
    long int historyenv = 0;
    int historylen = -1;

    // These can also be set via command line. 
    const char *historyfile = getenv("SPLINTER_HISTORY_FILE");
    const char *tmp = getenv("SPLINTER_HISTORY_LEN");
    
    // We set history length initially from env if appropriate
    if (tmp != NULL) historyenv = strtol(tmp, &buff, 10);
    if (historyenv <= INT_MAX) {
        historylen = historyenv;
    } else {
        fprintf(stderr, "Warning: SPLINTER_HISTORY_LEN env variable value (%ld) exceeds INT_MAX; ignoring.",
            historyenv);
    }
    if (historylen >= 0) linenoiseHistorySetMaxLen(historylen);

    // If you absolutely need to disable the implicit mode check based on invocation, 
    // comment out m = select_mode() and allow arguments to decide it exclusively.
    m = select_mode(progname);

    // input processors add to history, we have to manage the rest.
    if (historyfile != NULL && historylen > 0) linenoiseHistoryLoad(historyfile);
    
    snprintf(prompt, 3, "# ");
    print_version_info(progname);

    if (m == MODE_REPL) {        
        fprintf(stderr,"To quit, press ctrl-c or ctrl-d.\n");
        linenoiseSetCompletionCallback(completion);
        linenoiseSetHintsCallback(hints);
        do {
            // unpack the arguments into an argv[] style array
            mod_args = cli_input_args(prompt, &_argc);
            
            // ctrl-c or ctrl-d
            if (mod_args == NULL) break;
            // user just pressed enter alone
            if (_argc == 0) {
                cli_free_argv(mod_args);
                continue;
            }

            idx = cli_find_module(mod_args[0]);
            if (idx >= 0) {
                rc = cli_run_module(idx, _argc, mod_args);
            } else {
                fprintf(stderr, "Unknown command: %s\n", mod_args[0]);
                rc = 1;
            }

            // Put everything away for the next turn
            cli_free_argv(mod_args);
            mod_args = NULL;
            _argc = 0;

            // Here is where any janitorial things like refreshing
            // atomic views could happen. The kaboose on the input train.

        } while (1);
    } else {
        int i;
        for (i = 0; argv[i]; i++) printf("main: [%d/%d]: %s\n", i, argc, argv[i]);
        if (argc > 0) { 
            // twiddle from getopt (todo)
            _argc = argc - 1;
            mod_args = cli_slice_args(argv, _argc);
            for (i = 0; mod_args[i]; i++) printf("mod:  [%d/%d]: %s\n", i, argc, mod_args[i]);
            idx = cli_find_module(mod_args[0]);
            if (idx >= 0) {
                buff = cli_rejoin_args(mod_args);
                linenoiseHistoryAdd(buff);
                free(buff);
                buff = NULL;
                rc = cli_run_module(idx, _argc, mod_args);
            } else {
                fprintf(stderr, "Unknown command: %s\n", mod_args[0]);
            }
            free(mod_args);
            mod_args = NULL;
            _argc = 0;
        } else {
            // temporary (help display coming)
            fprintf(stderr, "Usage: %s [args] <command>\n", progname);
            rc = 1;
        }
    }

    // we've at least tried to process something at this point, so save history.
    // history is logged from both interactive and non-interactive modes.
    if (historyfile != NULL && historylen > 0) linenoiseHistorySave(historyfile);
    
    // linenoise's atexit doesn't get entered at normal termination, so explicitly
    // free history prior to exiting if in non-repl mode (just a nit)
    if (m == MODE_NO_REPL) {
        if (history) freeHistory();
    }
    
    return rc;
}
