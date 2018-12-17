#ifndef PANIC_H
#define PANIC_H

#include <stdio.h>
#include <stdlib.h>

#define ALLOC_ERROR {char message[32];sprintf(message, "cannot alloc at %s:%d", __FILE__, __LINE__);panic(message);}

inline void panic(const char* message) {
    printf("\033[31mError:\033[0m %s\n", message);
    exit(-1);
}

#endif /* PANIC_H */
