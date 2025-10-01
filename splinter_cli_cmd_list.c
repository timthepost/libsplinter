#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "list";

#define LIST_CMD_MAX_KEYS 2048

void help_cmd_list(unsigned int level) {
    (void) level;
    printf("%s lists keys in the currently selected store.\n", modname);
    printf("Usage: %s [pattern] [max_lines]\n", modname);
    return;
}

/*

  // Test 13: List keys
  char *key_list[10];
  size_t key_count;
  TEST("list keys", splinter_list(key_list, 10, &key_count) == 0);
*/

int cmd_list(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    char *entries[LIST_CMD_MAX_KEYS] = { 0 };
    size_t entry_count = 0;
    int rc = -1, i;

    rc = splinter_list(entries, LIST_CMD_MAX_KEYS, &entry_count);
    if (rc == 0) {
        for (i =0; entries[i]; i++) {
            if (entries[i][0] != '\0')
                printf(" %s\n", entries[i]);
        }
        puts("");
        return 0;
    }

    return rc;
}