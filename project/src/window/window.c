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
message_t buildInfoResponseWindow(long to_pid, short state, long open_time);

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

            if (msg.type == DELETE_MSG_TYPE) {
                message_t m = buildDeleteResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } else if (msg.type == INFO_MSG_TYPE) {
                time_t now = time(NULL);
                unsigned long work_time = open_time + (now - ((stato == 0) ? now : last_open_time));  //se è chiusa ritorno solo on_time, altrimenti on_time+tempo da quanto accesa
                message_t m = buildInfoResponseWindow(msg.sender, stato, open_time);
                sendMessage(&m);
            } else if (msg.type == SWITCH_MSG_TYPE) {
                int success = -1;
                if (msg.vals[SWITCH_VAL_LABEL] == 0) {    // interruttore (generico)
                    if (msg.vals[SWITCH_VAL_POS] == 0) {  // chiudo
                        stato = 0;
                        success = 0;

                        open_time += time(NULL) - last_open_time;
                        last_open_time = 0;
                    } else if (msg.vals[SWITCH_VAL_POS] == 1) {  // apro
                        stato = 1;
                        success = 0;

                        last_open_time = time(NULL);
                    }
                }
                // return success or not
                message_t m = buildSwitchResponse(success, msg.sender);
                sendMessage(&m);
            } else if (msg.type == TRANSLATE_MSG_TYPE) {
                message_t m = buildTranslateResponse(msg.sender, msg.vals[TRANSLATE_VAL_ID] == id ? 1 : 0);
                sendMessage(&m);
            } else if (msg.type == LIST_MSG_TYPE) {
                message_t m = buildListResponse(msg.sender, WINDOW, id, msg.vals[0], stato, 1);
                sendMessage(&m);
            }
        }
    }
    return 0;
}

message_t buildInfoResponseWindow(long to_pid, short state, long open_time) {
    message_t ret = buildInfoResponse(to_pid, WINDOW);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[1] = open_time;
    return ret;
}
