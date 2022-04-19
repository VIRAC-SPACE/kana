#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/utils.h"
#include "../include/panic.h"

char* getFileDirectory(char* filepath) {
    char* lastSlash = strrchr(filepath, '/');
    int dirlen = (int)(lastSlash - filepath);
    if (lastSlash == 0) dirlen = 0;
    char* directory = malloc(dirlen + 1);
    memcpy(directory, filepath, dirlen);
    directory[dirlen] = '\0';
    return directory;
}

char** splitString(const char* str, const char delim) {
    int count = 0;
    const char* tmp = str;
    const char* lastComma = NULL;
    while (*tmp) {
        if (delim == *tmp) {
            ++count;
            lastComma = tmp;
        }
        ++tmp;
    }
    count += lastComma < (str + strlen(str) - 1); // trailig token
    ++count; // terminating null

    char** result = malloc(sizeof(char*) * count);
    // if (result == NULL) ALLOC_ERROR;

    size_t idx  = 0;
    char delimComp[2] = {delim, 0};
    char* strcopy = strdup(str);
    char* token = strtok(strcopy, delimComp);

    while (token) {
        assert(idx < count);
        *(result + idx++) = strdup(token);
        token = strtok(0, delimComp);
    }
    assert(idx == count - 1);
    *(result + idx) = 0;

    return result;
}

char* strToUpper(char* str) {
    for (int i = 0; i < strlen(str); ++i) {
        str[i] = toupper(str[i]);
    }
    return str;
}

void changeFileExt(char* path, const char* ext) {
    char* dotpos = strrchr(path, '.');
    strcpy(dotpos, ext);
}

long double readUnixTime(char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        panic("couldn't open file");
    }

    char* year = malloc(sizeof(char) * 2);
    char* month = malloc(sizeof(char) * 2);
    char* day = malloc(sizeof(char) * 2);
    char* hh = malloc(sizeof(char) * 2);
    char* mm = malloc(sizeof(char) * 2);
    char* ss = malloc(sizeof(char) * 2);

    fseek(file, 35, SEEK_SET);
    if(fread(month, 1, 2, file) != 2) {
        panic("cannot read time data");
    }
    fseek(file, 38, SEEK_SET);
    if(fread(day, 1, 2, file) != 2) {
        panic("cannot read time data");
    }
    fseek(file, 41, SEEK_SET);
    if(fread(year, 1, 2, file) != 2) {
        panic("cannot read time data");
    }
    fseek(file, 43, SEEK_SET);
    if(fread(hh, 1, 2, file) != 2) {
        panic("cannot read time data");
    }
    fseek(file, 46, SEEK_SET);
    if(fread(mm, 1, 2, file) != 2) {
        panic("cannot read time data");
    }
    fseek(file, 49, SEEK_SET);
    if(fread(ss, 1, 2, file) != 2) {
        panic("cannot read time data");
    }

    struct tm UTCtime;
    UTCtime.tm_year = 2000 + atoi(year) - 1900;
    UTCtime.tm_mon = atoi(month)-1;
    UTCtime.tm_mday = atoi(day);
    UTCtime.tm_hour = atoi(hh);
    UTCtime.tm_min = atoi(mm);
    UTCtime.tm_sec = atoi(ss);
    UTCtime.tm_isdst = -1;

    free(month);
    free(day);
    free(year);
    free(hh);
    free(mm);
    free(ss);
    fclose(file);
    return (long double)mktime(&UTCtime);
}

inline double timeToSecs(char* time) {
    double secs = 0.0;
    char* hpos = strchr(time, 'h');
    char* mpos = strchr(time, 'm');
    char* spos = strchr(time, 's');

    char* ptime = strdup(time);
    if (hpos != NULL) {
        secs = secs + atol(strtok(ptime, "h")) * 3600.0;
        ptime = NULL;
    }
    if (mpos != NULL) {
        secs = secs + atol(strtok(ptime, "m")) * 60.0;
        ptime = NULL;
    }
    if (spos != NULL) {
        secs = secs + atof(strtok(ptime, "s"));
    }

    return secs;
}

int isFile(struct dirent* entry, char* dir) {
    char* filePath = malloc(strlen(dir) + strlen(entry->d_name) + 1);
    sprintf(filePath, "%s%s", dir, entry->d_name);

    struct stat pathStat;
    stat(filePath, &pathStat);
    free(filePath);
    return S_ISREG(pathStat.st_mode);
}

int isKanaFile(struct dirent* entry, char* dir, char* station) {
    int hasCorrectName = 0;
    char** split = splitString(entry->d_name, '_');
    for (int i = 0; split[i]; ++i) {
        char* tok = strToUpper(strdup(split[i]));
        char* sig = strToUpper(strdup(station));
        if (strcmp(tok, sig) == 0) {
            hasCorrectName = 1;
        }
        free(tok);
        free(sig);
        free(split[i]);
    }
    free(split);

    if (isFile(entry, dir) && hasCorrectName) {
        return 1;
    } else {
        return 0;
    }
}

unsigned long getFileSize(char* filePath) {
    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file to get size: %s", filePath);
        exit(-1);
    }

    fseek(file, 0, SEEK_END); // seek to end of file
    unsigned long size = ftell(file);
    rewind(file);
    fclose(file);
    return size;
}