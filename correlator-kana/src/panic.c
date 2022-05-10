#include <stdio.h>
#include <stdlib.h>

#include "../include/panic.h"

void panic(const char* message) {
    printf("\033[31mError:\033[0m %s\n", message);
    exit(-1);
}
