#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "list";

#define LIST_CMD_MAX_KEYS 150

void help_cmd_list(unsigned int level) {
    (void) level;
    printf("%s lists keys in the currently selected store.\n", modname);
    printf("Usage: %s [pattern] [max_lines]\n", modname);
    return;
}

static int compare_slots_by_epoch(const void *a, const void *b) {
    const splinter_slot_snapshot_t *slot_a = (const splinter_slot_snapshot_t *)a;
    const splinter_slot_snapshot_t *slot_b = (const splinter_slot_snapshot_t *)b;
    
    // Descending order: b - a
    if (slot_b->epoch > slot_a->epoch) return 1;
    if (slot_b->epoch < slot_a->epoch) return -1;
    
    return 0;
}

int cmd_list(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    splinter_slot_snapshot_t slots[LIST_CMD_MAX_KEYS] = { 0 };
    char *keynames[LIST_CMD_MAX_KEYS] = { 0 };
    size_t entry_count = 0;
    int rc = -1, i, x = 0; 

    rc = splinter_list(keynames, LIST_CMD_MAX_KEYS, &entry_count);
    if (rc == 0) {
        for (i =0; keynames[i]; i++) {
            if (keynames[i][0] != '\0') {
                splinter_get_slot_snapshot(keynames[i], &slots[x]);
                x++;
            }
        }

        qsort(slots, x, sizeof(splinter_slot_snapshot_t), compare_slots_by_epoch);

        printf("%-33s | %-15s | %-15s\n",
            "Key Name",
            "Epoch",
            "Value Length"
        );
        for (i = 0; i < 66; i++)
            putchar('-');
        putchar('\n');
        for (i = 0; slots[i].epoch > 0; i++) {
            printf("%-33s | %-15lu | %-15u\n", 
                slots[i].key,
                slots[i].epoch,
                slots[i].val_len
            );
        }
        puts("");
        return 0;
    }

    return rc;
}