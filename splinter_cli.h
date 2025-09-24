#ifndef SPLINTER_CLI_H
#define SPLINTER_CLI_H

#include "splinter.h"
#include <stdbool.h>

typedef struct cli_user {
    // contains the name of selected store + formatted errorlevel / debug info
    char prompt[128];
    // last returned errorlevel
    int32_t last_error;
    // last returned error message, if any (null otherwise)
    char *last_error_message;
    // whether or not this is an interactive user
    bool is_interactive;

    // Still needed: editline history
} cli_user_t;

typedef struct cli_state {
    // Do we have an open store?
    bool online;
    // What is the path name?
    char *current;
} cli_state_t;


#endif // SPLINTER_CLI_H