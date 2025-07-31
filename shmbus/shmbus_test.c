#include "shmbus.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    if (shmbus_create("/runa_bus", 256, 4096) != 0) {
        perror("create");
        return 1;
    }

    shmbus_set("hello", "world", 6);

    char buf[64]; size_t len;
    shmbus_get("hello", buf, sizeof buf, &len);
    printf("hello → %.*s (len=%zu)\n", (int)len, buf, len);

    shmbus_close();
    return 0;
}
