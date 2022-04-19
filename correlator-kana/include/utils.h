#ifndef KANA_UTILS_H
#define KANA_UTILS_H

#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>


char* getFileDirectory(char* filepath);
char** splitString(const char* str, const char delim);
char* strToUpper(char* str);
void changeFileExt(char* path, const char* ext);
long double readUnixTime(char* filename);
double timeToSecs(char* time);
int isFile(struct dirent* entry, char* dir);
int isKanaFile(struct dirent* entry, char* dir, char* station);
unsigned long getFileSize(char* filePath);

#endif //KANA_UTILS_H
