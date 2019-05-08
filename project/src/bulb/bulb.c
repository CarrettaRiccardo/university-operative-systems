/*
TODO: Gestire correttamente il tempo che  rimasta accesa
TODO: Gestire i vari stati : 0=spenta 1=accesa 2=spenta manually 3=accesa manually
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"

/* Override specifico per il metodo definito in IPC */
message_t buildInfoResponseBulb(const long id, const short stato,
                                const int to, const char *tipo_componente,
                                const long work_time);

int main(int argc, char **argv) {
    const int id = atoi(argv[1]);           // Lettura id da parametro
    short stato = 0;                        // 0 = spenta, 1 = accesa
    unsigned int on_time = 0;   //TODO: Destro fare lettura on_time da parametro in caso di clonazione
    unsigned long last_start_time = time(NULL);  // Tempo accensione lampadina

    while (1) {
        message_t msg;
        int result = receiveMessage(getpid(), &msg);
        if (result == -1) {
            perror("BULB: Errore ricezione");
        } else {
            if (msg.to == -1) continue;  // Messaggio da ignorare (per sessione diversa/altri casi)

            if (strcmp(msg.text, MSG_DELETE_REQUEST) == 0) {
                message_t m = buildDieResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } else if (strcmp(msg.text, INFO_REQUEST) == 0) {
                time_t now = time(NULL);
                unsigned long work_time = on_time + (now - ( (stato==0) ? now : last_start_time)  ); //se è spenta ritorno solo on_time, altrimenti on_time+tempo da quanto accesa
                message_t m = buildInfoResponseBulb(id, stato, msg.sender, "Bulb", work_time);
                sendMessage(&m);
            } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
                int success = -1;
                if (msg.value1 == 0){// interruttore (generico)
                    if (msg.value2 == 0){// spengo
                        stato = 0;
                        success = 0;

                        on_time += time(NULL) - last_start_time;
                        last_start_time = 0;
                    }
                    if (msg.value2 == 1){// accendo
                        stato = 1;
                        success = 0;

                        last_start_time = time(NULL);
                    }
                }
                // return success or not
                message_t m = buildSwitchResponse(success, msg.sender);
                sendMessage(&m);
            } else if (strcmp(msg.text, MSG_TRANSLATE) == 0) {
                message_t m = buildTranslateResponse(id, msg.value1, msg.sender);
                sendMessage(&m);
            } else if (strcmp(msg.text, MSG_LIST) == 0) {  // Caso base per la LIST. value5 = 1 per indicare fine albero
                message_t m = buildListResponse(msg.sender, "Bulb", stato, msg.value1, 1, id);
                sendMessage(&m);
            }
        }
    }
    return 0;
}

message_t buildInfoResponseBulb(const long id, const short stato, const int to, const char *tipo_componente, const long work_time) {
    message_t ret = buildInfoResponse(id, stato, to, tipo_componente);
    ret.value1 = work_time;
    return ret;
}
