#ifndef SPLINTER_CLI_H
#define SPLINTER_CLI_H

#include "splinter.h"
#include <stdbool.h>

// Types for module command entry and help 
typedef int (*mod_entry_t)(int, char *[]);
typedef void (*mod_help_t)(unsigned int);

// A command (module)
typedef struct cli_module {
    // correlates to array position for easy lookup
    int id;
    // name of the command, e.g. set / get / help
    const char *name;
    // computed length of the command to speed up linear probing
    size_t name_len;
    // What it does
    const char *description;
    // Is this just another name for another command? Set this to its id.
    int alias_of;
    // If alias, set these to null (we resolve the root entry points)
    mod_entry_t entry;
    mod_help_t help;
} cli_module_t;

// Prototypes for runtime functions
int cli_handle_input(int async, const char *prompt);
char **cli_unroll_argv(const char *input, int *out_argc);
void cli_free_argv(char *argv[]);
int cli_find_module(const char *name);
int cli_find_alias(int idx);
int cli_run_module(int idx);
void cli_show_module_help(int idx);

// Prototypes for individual command entry points
int cmd_help(int argc, char *argv[]);
void help_cmd_help(unsigned int level);

// And finally an array of modules to hold them all
extern cli_module_t command_modules[];

#endif // SPLINTER_CLI_H
