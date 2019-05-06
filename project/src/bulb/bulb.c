#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../include/ipc.h"  //TODO: Destro linkare libreria ipc.c

int main(int argc, char **argv) {
    const int id = 7; //TODO: Leggere id da parametro
    
    short stato = 0;  //0 = spenta, 1 = accesa
    unsigned long start_time = time(NULL); //tempo accensione lampadina
    
    while (1){
        message_t msg = receiveMessage(getpid());

        if(msg.to == -1) //messaggio da ignorare (per sessione diversa/altri casi)
            continue;

        if (strcmp(msg.text, "DIE") == 0){
            message_t m = buildDieResponse(msg.sender);
            sendMessage(m);
            exit(0);
        }
        else if (strcmp(msg.text, "INFO") == 0){
            unsigned long work_time = time(NULL) - start_time;
            message_t m = buildInfoResponse(id,work_time ,stato,msg.sender," bulb");
            sendMessage(m);
        }
        else if (strcmp(msg.text, "TRANSLATE") == 0){
            message_t m = buildTranslateResponse(id,msg.value, msg.sender);
            sendMessage(m);
        }
    }

    return 0;
}