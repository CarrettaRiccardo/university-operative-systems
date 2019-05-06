#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../include/ipc.h"  //TODO: Destro linkare libreria ipc.c

//Override specifico per il metodo definito in IPC
message_t buildInfoResponseBulb(const long id, const short stato, 
                            const int to, const char *tipo_componente,
                            const long work_time);

int main(int argc, char **argv)
{
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
            message_t m = buildInfoResponseBulb(id, stato, msg.sender, "Bulb", work_time);
            sendMessage(m);
        }
        else if (strcmp(msg.text, "TRANSLATE") == 0){
            message_t m = buildTranslateResponse(id,msg.value1, msg.sender);
            sendMessage(m);
        }
        else if (strcmp(msg.text, "LIST") == 0){ //caso base per la LIST. value5=1 per indicare fine albero
            message_t m = buildListResponse(msg.sender,"Bulb", stato, msg.value1, 1, id);
            sendMessage(m);
        }
    }

    return 0;
}

message_t buildInfoResponseBulb(const long id, const short stato, const int to, const char *tipo_componente, const long work_time)
{
    message_t ret = buildInfoResponse(id,stato,to,tipo_componente);
    ret.value1 = work_time;
    return ret;
}