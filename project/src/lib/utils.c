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

// stampa nel file con nome della session il messaggio
int printLog(message_t msg){
    char f_name[30];
    int ret = -1;
    strcpy(f_name,strcat(msg.session,".txt"));
    FILE* log = fopen(f_name, "a");// crea se non esiste
    if (log != NULL){
        fprintf(log, "TYPE:%s | FROM:%ld | TO:%ld | VALUES:%ld, %ld, %ld, %ld, %ld, %ld\n", msg.text, msg.sender, msg.to, msg.value1, msg.value2, msg.value3, msg.value4, msg.value5, msg.value6);
        // chiudo subito per evitare conflitti di apertura
        fclose(log);
        ret = 0;
    }
    else{
        // error opening file
    }
    return ret;
}
