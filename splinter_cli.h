#ifndef SPLINTER_CLI_H
#define SPLINTER_CLI_H

#include "splinter.h"
#include <stdbool.h>

/* Types for module command entry and help */
typedef int (*mod_entry_t)(int, char *[]);
typedef void (*mod_help_t)(unsigned int);

typedef struct cli_module {
    const char *name;
    const char *description;
    bool is_alias;
    mod_entry_t entry;
    mod_help_t help;
} cli_module_t;

// Prototypes for runtime functions
int cli_handle_input(int async, const char *prompt);

// Prototypes for individual command entry points
int cmd_help(int argc, char *argv[]);
void help_cmd_help(unsigned);

extern cli_module_t command_modules[];

#endif // SPLINTER_CLI_H
