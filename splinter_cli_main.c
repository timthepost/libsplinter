#include <stdio.h>
#include <string.h>
#include "splinter_cli.h"

int main (int argc, char *argv[]) {
    puts("Hello from splinter_cli.");
    printf("Argument 0 is '%s', and %d arguments were passed.\n", argv[0], argc);
    return 0;
}