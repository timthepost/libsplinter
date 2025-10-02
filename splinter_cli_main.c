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
 * todo for release:
 *  - hist command
 *  - export command
 *  - create command (currently missing)
 *  - maybe include something fun? 
 */

/**
 * We deviate from linenoise default here (which hides these)
 * but we need direct access to the history array for non-repl
 * cleanup on exit (non-repl gets logged in history, too, but 
 * different handlers when not getting direct input). 
 */
extern char **history;
extern void freeHistory(void);

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
        "config",
        6,
        "Access Splinter bus and slot metadata.",
        -1,
        &cmd_config,
        &help_cmd_config
    },
    {
        3,
        "export",
        6,
        "Export store contents and metadata to JSON.",
        -1,
        &cmd_export,
        &help_cmd_export
    },
    {
        4,
        "get",
        3,
        "Retrieve the value of a key in the store.",
        -1,
        &cmd_get,
        &help_cmd_get
    },
    {
        5,
        "head",
        4,
        "Retrieve just the metadata of a key in the store.",
        -1,
        &cmd_head,
        &help_cmd_head
    },
    {
        6,
        "help",
        4,
        "Help with commands and features.",
        -1,
        &cmd_help,
	    &help_cmd_help
    },
    {
        7,
        "hist",
        4,
        "View and clear command history.",
        -1,
        &cmd_hist,
        &help_cmd_hist
    },
    {
        8,
        "list",
        4,
        "List keys in the current store.",
        -1,
        &cmd_list, 
        &help_cmd_list
    },
    {
        9,
        "set",
        3,
        "Set a key in the store to a specified value.",
        -1,
        &cmd_set,
        &help_cmd_set
    },
    {
        10,
        "unset",
        5,
        "Unset a key in the store (deletes the key).",
        -1,
        &cmd_unset,
        &help_cmd_unset
    },
    {
        11,
        "use",
        3,
        "Opens a Splinter store by name or path.",
        -1,
        &cmd_use,
        &help_cmd_use
    },
    {
        12,
        "watch",
        5,
        "Observes a key for changes and prints updated contents.",
        -1,
        &cmd_watch,
        &help_cmd_watch
    },
    // The last null-filled element 
    { 0, NULL, 0, NULL, -1,  NULL , NULL }
};

// Initialize a shared session structure
// (declared as extern in splinter_cli.h)
cli_user_t thisuser = {
    { 0 },
    false,
    0,
    {0},
    0,
    0
};

// We raise thisuser.abort on SIGUSER1 (so far)
static void cli_handle_signal(int signum) {
    switch (signum) {
        // This arrangement allows using them as either an emergency stop, 
        // or stop / start button. 
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
    fprintf(stderr, 
        "Usage:  %s [options] [aguments] *or*\n\t%s --no-repl <built in command> [arguments] *or*\n\t%s {no args for REPL}\n",
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
    fprintf(stderr, "  --use / -u <store>           Connect to <store> after starting.\n");
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
            linenoiseAddCompletion(lc, "cls");
            linenoiseAddCompletion(lc, "clear");
            linenoiseAddCompletion(lc, "config");
            break;
        case 'e':
            linenoiseAddCompletion(lc, "export");
            break;
        case 'g':
            linenoiseAddCompletion(lc, "get");
            break;
        case 'h':
            linenoiseAddCompletion(lc, "help");
            linenoiseAddCompletion(lc, "head");
            linenoiseAddCompletion(lc, "hist");
            break;
        case 'l':
            linenoiseAddCompletion(lc, "list");
            break;
        case 's':
            linenoiseAddCompletion(lc, "set");
            break;
        case 'u':
            linenoiseAddCompletion(lc, "use");
            linenoiseAddCompletion(lc, "unset");
            break;
        case 'w':
            linenoiseAddCompletion(lc, "watch");
            break;
        default:
            break;
    }

    return;
}

// callback that provides completion hints while typing
// TODO re-think how this works; it's onerous to maintain and 
// kinda clunky.
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
        // I made aliases magenta. You can make them whatever you want.
        *color = 35;
        *bold = 1;
        return "s ";
    }

    if (!strncasecmp(buf, "cle", 5)) {
        *color = 36;
        *bold = 1;
        return "ar ";
    }

    if (!strncasecmp(buf, "co", 6)) {
        *color = 36;
        *bold = 1;
        return "nfig ";
    }

    if (!strncasecmp(buf, "e", 6)) {
        *color = 36;
        *bold = 1;
        return "xport ";
    }

    if (!strncasecmp(buf, "g", 3)) {
        *color = 36;
        *bold = 1;
        return "et ";
    }

    if (!strncasecmp(buf, "l", 4)) {
        *color = 36;
        *bold = 1;
        return "ist ";
    }

    if (!strncasecmp(buf, "hi", 4)) {
        *color = 36;
        *bold = 1;
        return "st ";
    }

    if (!strncasecmp(buf, "hea", 4)) {
        *color = 36;
        *bold = 1;
        return "d ";
    }

    if (!strncasecmp(buf, "hel", 4)) {
        *color = 36;
        *bold = 1;
        return "p ";
    }

    if (!strncasecmp(buf, "s", 3)) {
        *color = 36;
        *bold = 1;
        return "et ";
    }

    // BUG
    // No idea why, but use and unset get swapped in autocomplete

    if (!strncasecmp(buf, "u", 3) && strncasecmp(buf, "n", 3)) {
        *color = 36;
        *bold = 1;
        return "se ";
    }

    if (!strncasecmp(buf, "un", 5)) {
        *color = 36;
        *bold = 1;
        return "set ";
    }

    if (!strncasecmp(buf, "w", 5)) {
        *color = 36;
        *bold = 1;
        return "atch ";
    }

    return NULL;
}

// TODO: Implement --use for --no-repl commands
static const struct option long_options[] = {
    { "help", optional_argument, NULL, 'h' },
    { "history-file", required_argument, NULL, 'H' },
    { "history-len", required_argument, NULL, 'l' },
    { "list-modules", no_argument, NULL, 'L' },
    { "no-repl", no_argument, NULL, 'n' },
    { "use", required_argument, NULL, 'u' },
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
    if (m == MODE_REPL)
        // restore from possibly being set in non-blocking mode
        tcsetattr(STDIN_FILENO, TCSANOW, &thisuser.term);
}

int main (int argc, char *argv[]) {
    int rc = 0, _argc = 0, idx = -1, opt, historylen = -1;
    char *progname = basename(argv[0]), *buff = NULL;
    char prompt[128] = { 0 };
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
    
    // Modules must know what this is in order to be able to restore it
    tcgetattr(STDIN_FILENO, &thisuser.term);

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
            // --use / -u
            case 'u':
                if (splinter_open(optarg) == 0) {
                    thisuser.store_conn = 1;
                    strncpy(thisuser.store, optarg, sizeof(thisuser.store) -1);
                } else {
                    fprintf(stderr, "%s: could not connect to '%s'; will start disconnected.\n",
                    progname, optarg);
                }
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
            // If this case is entered, it means getopt_long() is likely broken?
            default:
                fprintf(stderr, "%s: unknown error parsing argument '%c'. Try %s --help for help.\n", 
                    progname,
                    optopt, 
                    progname);
                exit(EXIT_FAILURE);
        }
    }

    if (m == MODE_REPL) {
        if (cli_set_signal_handlers() < 0) {
            fprintf(stderr, 
                "%s: failed to register signal handlers. Certain interactive features may malfunction.\n",
                progname);
        }
        
        print_version_info(progname);        
        fprintf(stderr,"To quit, press ctrl-c or ctrl-d.\n");
        
        linenoiseSetCompletionCallback(completion);
        linenoiseSetHintsCallback(hints);
        
        do {
            if (thisuser.lasterrno != 0) {
                snprintf(prompt, 128, "%d : %s # ",
                    thisuser.lasterrno,
                    thisuser.store[0] == '\0' ? "no-conn" : thisuser.store
                );
            } else {
                snprintf(prompt, 128, "%s # ",
                    thisuser.store[0] == '\0' ? "no-conn" : thisuser.store
                );
            }
            // unpack the arguments into an argv[] style array
            mod_args = cli_input_args(prompt, &_argc);
            
            // ctrl-c or ctrl-d
            if (mod_args == NULL) break;

            // user just pressed enter alone
            if (_argc == 0) {
                cli_free_argv(mod_args);
                continue;
            }

            // there actually might be something to do
            idx = cli_find_module(mod_args[0]);
            if (idx >= 0) {
                rc = cli_run_module(idx, _argc, mod_args);
            } else {
                fprintf(stderr, "Unknown command: %s\n", mod_args[0]);
                rc = 1;
            }

            thisuser.lasterrno = rc;

            // re-set for next turn
            cli_free_argv(mod_args);
            mod_args = NULL;
            _argc = 0;

            // End of REPL loop
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

    if (historyfile != NULL && historylen > 0) linenoiseHistorySave(historyfile);
    
    // Since we didn't invoke linenoise in non-repl mode, we won't benefit from its 
    // atexit function. So, we have to call it explicitly in non-repl mode. (this is
    // also a change I made to line noise, history was static (opaque))
    if (m == MODE_NO_REPL) {
        if (history) freeHistory();
    }
    
    return rc;
}
