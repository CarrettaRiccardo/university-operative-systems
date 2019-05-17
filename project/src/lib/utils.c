#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/constants.h"

int isInt(char *str) {
    if (*str == '-') str++;  // Ignoro un eventuale meno come primo carattere
    while (*str != '\0') {
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return 1;
}

char *extractBaseDir(char *path) {
    char *base_dir = malloc(sizeof(path) + MAX_DEVICE_NAME_LENGTH);
    strcpy(base_dir, path);
    char *last_slash = strrchr(base_dir, '/');
    if (last_slash) *(last_slash + 1) = '\0';
    return base_dir;
}
