// splinter_test.c
#include "splinter.h"
#include <stdio.h>
#include <string.h>

#define BUS_NAME "/runa_debug"

int main() {
    if (splinter_create(BUS_NAME, 32, 1024) != 0) {
        perror("splinter_create");
        printf("The \"%s\" bus is in-use and can't be created, attempting open instead.\n", BUS_NAME);
	if (splinter_open(BUS_NAME) != 0) {
		perror("splinter_open");
		return 1;
	}
    }

    const char *key = "runa_response";
    const char *val = "hello";
    if (splinter_set(key, val, strlen(val) + 1) != 0) {
        perror("splinter_set");
        return 1;
    }

    char buf[1024];
    size_t out_sz = 0;
    if (splinter_get(key, buf, sizeof(buf), &out_sz) != 0) {
        perror("splinter_get");
        return 1;
    }

    printf("%s \u2192 %s (len=%zu)\n", key, buf, out_sz);
    splinter_close();
    return 0;
}
