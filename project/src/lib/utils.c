#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>

int isInt(char *str) {
    while (*str != '\0') {
        if (*str < '0' || *str > '9') return 0;
        str++;
    }
    return 1;
}
