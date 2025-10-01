#include <stdio.h>
#include <string.h>
#include "splinter_cli.h"

static const char *modname = "config";

void help_cmd_config(unsigned int level) {
    printf("Usage: %s\n       %s <key_name>\n", modname, modname);
    if (level)
        printf("%s is currently a read-only tool.\n", modname);
    return;
}

static void show_bus_config(void) {
    splinter_header_snapshot_t snap = {0};

    splinter_get_header_snapshot(&snap);

    printf("magic:       %u\n", snap.magic);
    printf("version:     %u\n", snap.version);
    printf("slots:       %u\n", snap.slots);
    printf("max_val_sz:  %u\n", snap.max_val_sz);
    printf("epoch:       %lu\n", snap.epoch);
    printf("auto_vacuum: %u\n", snap.auto_vacuum);
    puts("");
    
    return;
}

/*
config (shows bus config)
config <keyname> (shows slot config value of keyname)
config set <bus_config_key> <value> (sets bus config <key> to <value>, if RW)
*/

int cmd_config(int argc, char *argv[]) {
    (void) argv;
    
    if (argc == 1) {
        show_bus_config();
        return 0;
    }

    if (argc == 2) {
        cli_show_key_config(argv[1], modname);
        return 0;
    }

    help_cmd_config(1);
    
    return 1;
}