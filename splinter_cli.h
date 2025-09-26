#ifndef SPLINTER_CLI_H
#define SPLINTER_CLI_H

#include "splinter.h"
#include <stdbool.h>

// Types for module command entry and help 
typedef int (*mod_entry_t)(int, char *[]);
typedef void (*mod_help_t)(unsigned int);

// A command (module)
typedef struct cli_module {
    const char *name;
    const char *description;
    bool is_alias;
    bool interactive_only;
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
