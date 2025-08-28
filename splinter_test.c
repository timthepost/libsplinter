// splinter_test.c
#include "splinter.h"
#include <stdio.h>
#include <string.h>

#define BUS_NAME "/runa_debug"

int main() {
    int chars_deleted = 0;
    char buf[1024];
    size_t out_sz = 0;
    const char *key = "runa_response";
    const char *val = "hello";

    if (splinter_create_or_open(BUS_NAME, 32, 1024) != 0) {
        perror("splinter_create");
		return 1;
    }

    if (splinter_set(key, val, strlen(val) + 1) != 0) {
        perror("splinter_set");
        return 1;
    }

    if (splinter_get(key, buf, sizeof(buf), &out_sz) != 0) {
        perror("splinter_get");
        return 1;
    }

    printf("%s \u2192 %s (len=%zu)\n", key, buf, out_sz);

    chars_deleted = splinter_unset(key);

    printf("%d characters unset.\n", chars_deleted);

    splinter_close();
    return 0;
}
