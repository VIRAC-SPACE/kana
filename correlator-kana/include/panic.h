#ifndef KANA_PANIC_H
#define KANA_PANIC_H

#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdio.h>

void panic(const char* message);

#endif //KANA_PANIC_H
