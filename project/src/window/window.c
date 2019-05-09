/*
TODO: Gestire i vari stati : 0=chiusa 1=aperta 2=chiusa manually 3=aprta manually
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"

/* Override specifico per il metodo definito in IPC */
message_t buildInfoResponseWindow(const long id, const short stato,
                                  const int to, const char *tipo_componente,
                                  const long open_time);

int main(int argc, char **argv) {
    const int id = atoi(argv[1]);               // Lettura id da parametro
    short stato = 0;                            // per significato vedi sopra
    unsigned int open_time = 0;                 //TODO: Destro fare lettura open_time da parametro in caso di clonazione
    unsigned long last_open_time = time(NULL);  // Tempo ultima apertura lampadina

    while (1) {
        message_t msg;
        int result = receiveMessage(&msg);
        if (result == -1) {
            perror("WINDOW: Errore ricezione");
        } else {
            if (msg.to == -1)
                continue;  // Messaggio da ignorare (per sessione diversa/altri casi)

            if (strcmp(msg.text, MSG_DELETE_REQUEST) == 0) {
                message_t m = buildDieResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } else if (strcmp(msg.text, INFO_REQUEST) == 0) {
                time_t now = time(NULL);
                unsigned long work_time = open_time + (now - ((stato == 0) ? now : last_open_time));  //se è chiusa ritorno solo on_time, altrimenti on_time+tempo da quanto accesa
                message_t m = buildInfoResponseWindow(id, stato, msg.sender, WINDOW, work_time);
                sendMessage(&m);
            } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
                int success = -1;
                if (msg.vals[0] == LABEL_OPEN_VALUE) {      // interruttore (apri/chiudi)
                    if (msg.vals[1] == SWITCH_POS_OFF_VALUE) {  // chiudo
                        stato = SWITCH_POS_OFF_VALUE;
                        success = 1;
                        on_time += time(NULL) - last_start_time;
                        last_start_time = 0;
                    }
                    if (msg.vals[1] == SWITCH_POS_ON_VALUE) {  // apro
                        stato = SWITCH_POS_ON_VALUE;
                        success = 1;
                        last_start_time = time(NULL);
                    }
                }
                // return success or not
                message_t m = buildSwitchResponse(success, msg.sender);
                sendMessage(&m);
            } else if (strcmp(msg.text, MSG_TRANSLATE) == 0) {
                message_t m = buildTranslateResponse(id, msg.vals[0], msg.sender);
                sendMessage(&m);
            } else if (strcmp(msg.text, MSG_LIST) == 0) {  // Caso base per la LIST. value5 = 1 per indicare fine albero
                message_t m = buildListResponse(msg.sender, WINDOW, stato, msg.vals[0], 1, id);
                sendMessage(&m);
            }
        }
    }
    return 0;
}

message_t buildInfoResponseWindow(const long id, const short stato, const int to, const char *tipo_componente, const long work_time) {
    message_t ret = buildInfoResponse(id, stato, to, tipo_componente);
    ret.vals[0] = work_time;
    return ret;
}
