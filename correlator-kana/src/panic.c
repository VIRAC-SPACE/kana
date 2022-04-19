#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/panic.h"

void panic(const char* message) {
    printf("\033[31mError:\033[0m %s\n", message);
    exit(-1);
}
