/**
 * Copyright 2025 Tim Post
 * License: Apache 2 (MIT available upon request to timthepost@protonmail.com)
 *
 * @file splinter_cli_main.c
 * @brief Splinter CLI main entry / completion & hint setup / parsing / executing
 */
#ifndef _GNU_SOURCE
// for getopt extensions
#define _GNU_SOURCE 
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>
#include <errno.h>

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
 *  - Implement all commands but "watch" (need to add clear & hist)
 *  - Implement ctrl - ] SIGUSR1 handler for exiting watches without exiting CLI
 *  - Implement "watch"
 *  - Test async input with watch
 *  - (Maybe) implement code for editor with "edit" command just to access for now?
 *  - (Maybe) implement some kind of test harness to test the commands?
 */

enum mode {
    MODE_REPL,
    MODE_NO_REPL
};

// whether we run one-shot or in REPL
static enum mode m = MODE_REPL;

// holds all of the commands
cli_module_t command_modules[] = {
    {
        0,
        "clear",
        5,
        "Clears the screen.",
        -1,
        &cmd_clear,
        &help_cmd_clear
    },
    {
        1,
        "cls",
        3,
        "Alias of 'clear'",
        0,
        &cmd_clear,
        &help_cmd_clear
    },    
    {
        2,
        "help",
        4,
        "Help with commands and features.",
        -1,
        &cmd_help,
	    &help_cmd_help
    },
    {
        3,
        "use",
        3,
        "Opens a Splinter store by name or path.",
        -1,
        &cmd_use,
        &help_cmd_use
    },
    // The last null-filled element 
    { 0, NULL, 0, NULL, -1,  NULL , NULL }
};

// Initialize session structure
cli_user_t thisuser = {
    false,
    0,
    0,
    0
};

// We raise thisuser.abort on SIGUSER1 (so far)
static void cli_handle_signal(int signum) {
    switch (signum) {
        // This arrangement allows using them as either an emergency stop, 
        // or stop / start button. Useful for handling "more"-like output.
        case SIGUSR1:
            thisuser.abort = 1;
            break;
        case SIGUSR2:
            thisuser.abort = 0;
            break;
        // don't interfere with linenoise
        default:
            break;
    }
}

static int cli_set_signal_handlers(void) {
    struct sigaction sa = {0};
    sa.sa_handler = cli_handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    int rc = 0;

    rc += sigaction(SIGUSR1, &sa, NULL);
    rc += sigaction(SIGUSR2, &sa, NULL);

    return rc;
}

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

void print_usage(char *progname) {
    fprintf(stderr, "%s provides a command line interface for Splinter bus interaction.\n", progname);
    fprintf(stderr, "Usage:  %s [options] [aguments] *or*\n\t%s --no-repl <built in command> [arguments] *or*\n\t%s {no args for REPL}\n",
        progname, 
        progname,
        progname
    );
    fprintf(stderr, "Where [options] are:\n");
    fprintf(stderr, "  --help / -h                  Show this help display.\n");
    fprintf(stderr, "  --history-file / -H <path>   Set the CLI history file to <path>\n");
    fprintf(stderr, "  --history-len / -l  <len>    Set the CLI history length to <len>\n");
    fprintf(stderr, "  --list-modules / -L          List available commands.\n");
    fprintf(stderr, "  --no-repl / -n               Don't enter interactive mode.\n");
    fprintf(stderr, "  --version / -v               Print splinter version information and exit.\n");
    fprintf(stderr, "\n%s will look for SPLINTER_HISTORY_FILE and SPLINTER_HISTORY_LEN in the\n", progname);
    fprintf(stderr, "environment and use them; however argument values will always take precedence.\n");
    fprintf(stderr, "\nIf invoked as \"splinterctl\", %s automatically turns on --no-repl.\n", progname);
    fprintf(stderr, "\nPlease report bugs to https://github.com/timthepost/libsplinter");
    return;
} 

// fires whenever the user presses tab for tab / auto completion
static void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == '\0') return;

    switch (buf[0]) {
        case 'c':
            linenoiseAddCompletion(lc, "clear");
            linenoiseAddCompletion(lc, "cls");
            break;
        case 'h':
            linenoiseAddCompletion(lc, "help");
            break;
        case 'u':
            linenoiseAddCompletion(lc, "use");
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

    if (!strncasecmp(buf, "cl", 3)) {
        *color = 32;
        *bold = 1;
        return "s";
    }

    if (!strncasecmp(buf, "cle", 5)) {
        *color = 36;
        *bold = 1;
        return "ar";
    }

    if (!strncasecmp(buf,"h", 4)) {
        *color = 36;
        *bold = 1;
        return "elp ";
    }

    if (!strncasecmp(buf,"u", 3)) {
        *color = 36;
        *bold = 1;
        return "se ";
    }

    return NULL;
}

static const struct option long_options[] = {
    { "help", optional_argument, NULL, 'h' },
    { "history-file", required_argument, NULL, 'H' },
    { "history-len", required_argument, NULL, 'l' },
    { "list-modules", no_argument, NULL, 'L' },
    { "no-repl", no_argument, NULL, 'n' },
    { "version", no_argument, NULL, 'v' },
    {NULL, 0, NULL, 0}
};

static const char *optstring = "h::H:l:Lnv";

// halts execution if strtol would overflow an integer.
static int safer_atoi(const char *string) {

    char *buff;
    long l;

    l = strtol(string, &buff, 10);
    if (l <= INT_MAX) {
        return (int) l;
    } else {
        fprintf(stderr, "Value or argument would overflow an integer. Exiting.\n");
        exit(EXIT_FAILURE);
    }
}

static void cli_at_exit(void) {
    if (thisuser.store_conn)
        splinter_close();
}

int main (int argc, char *argv[]) {
    int rc = 0, _argc = 0, idx = -1, opt, historylen = -1;
    char *progname = basename(argv[0]), *buff = NULL;
    char prompt[64] = { 0 };
    char **mod_args = { 0 };

    // These can also be set via command line. 
    char *historyfile = getenv("SPLINTER_HISTORY_FILE");
    const char *tmp = getenv("SPLINTER_HISTORY_LEN");
    
    // We set history length initially from env if appropriate
    if (tmp != NULL) historylen = safer_atoi(tmp);

    if (historylen >= 0) linenoiseHistorySetMaxLen(historylen);
    if (historyfile != NULL && historylen > 0) linenoiseHistoryLoad(historyfile);

    // If you absolutely need to disable the implicit mode check based on invocation, 
    // comment out m = select_mode() and allow arguments to decide it exclusively.
    m = select_mode(progname);
    atexit(cli_at_exit);
    
    // parse arguments here
    opterr = 0;
    while ((opt = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
        switch (opt) {
            // --help / -h
            case 'h':
                print_usage(progname);
                exit(EXIT_SUCCESS);
            // --history-file / -H
            case 'H':
                linenoiseHistoryLoad(optarg);
                break;
            // --history-len / -l
            case 'l':
                linenoiseHistorySetMaxLen(safer_atoi(optarg));
                break;
            // --list-modules / -L
            case 'L':
                print_version_info(progname);
                cli_show_modules();
                exit(EXIT_SUCCESS);
            // --no-repl / -n
            case 'n':
                m = MODE_NO_REPL;
                break;
            // --version / -v
            case 'v':
                print_version_info(progname);
                exit(EXIT_SUCCESS);
            // argument requires option
            case ':':
            case '?':
                fprintf(stderr, "%s: option '-%c' requires an argument. Try %s --help for help.\n", 
                    progname, 
                    optopt, 
                    progname);
                exit(EXIT_FAILURE);
            // If this case is entered, it means getopt_long() is likely broken.
            default:
                fprintf(stderr, "%s: unknown error parsing argument '%c'. Try %s --help for help.\n", 
                    progname,
                    optopt, 
                    progname);
                exit(EXIT_FAILURE);
        }
    }

    // if optind > argc here, there are additional args, which we can unpack
    // later in NO_REPL mode. We don't consider them if we're in REPL mode,
    // so address that first.

    if (m == MODE_REPL) {
        if (cli_set_signal_handlers() < 0) {
            fprintf(stderr, "%s: failed to register signal handlers. Certain interactive features may malfunction.\n",
                progname);
        }
        snprintf(prompt, 3, "# ");
        print_version_info(progname);        
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
        if (argc > 1) { 
            _argc = argc - optind;
            mod_args = cli_slice_args(argv, _argc);
            if (mod_args[0]) {
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
            }
            free(mod_args);
            mod_args = NULL;
            _argc = 0;
        } else {
            print_usage(progname);
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
