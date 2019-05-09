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
    short stato = SWITCH_POS_OFF_VALUE;         // per significato vedi sopra
    short interruttore = SWITCH_POS_OFF_VALUE;  // valore interruttore (torna subito ad off (sempre off), ma se azionato apre/chiude la finestra in modo inverso)
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
                if(getppid() != msg.sender){ //sto morendo, invio conferma di ricezione al mittente, e nel caso che il mittente non sia mio padre, invio un messaggio a mio padre di rimuovermi dalla lista dei suoi figli
                    message_t m = buildDieMessage(getppid());
                    sendMessage(&m);
                }
                exit(0);
            } else if (msg.type == INFO_MSG_TYPE) {
                time_t now = time(NULL);
                unsigned long work_time = open_time + (now - ((stato == 0) ? now : last_open_time));  //se è chiusa ritorno solo "open_time", altrimenti open_time+differenza da quanto accesa
                message_t m = buildInfoResponseWindow(msg.sender, stato, work_time);
                sendMessage(&m);
            } else if (msg.type == SWITCH_MSG_TYPE) {
                int success = -1;
                if (msg.vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE || msg.vals[SWITCH_VAL_LABEL] == LABEL_GENERIC_SWITCH_VALUE) { // interruttore (apri/chiudi) o generico (da hub ai propri figli)
                    // apro/chiudo (invertendo) solo se preme "on" in quanto l'interruttore sarà sempre "off"
                    if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {
                        // se è chiuso, apro e salvo il tempo di apertura
                        if (stato == SWITCH_POS_OFF_VALUE){
                            last_open_time = time(NULL);
                            stato = SWITCH_POS_ON_VALUE;
                            interruttore = SWITCH_POS_OFF_VALUE;
                        } else {
                            // se è aperto, sommo il tempo di apertura e chiudo
                            if (stato == SWITCH_POS_ON_VALUE){
                                open_time += time(NULL) - last_open_time;
                                stato = SWITCH_POS_OFF_VALUE;
                                interruttore = SWITCH_POS_OFF_VALUE;
                            }
                        }
                        success = 1;
                    } else{ 
                        // se inserisce "off" non deve fare nulla in quanto l'interruttore è sempre a "off"
                        if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {
                            success = 1;
                        }
                    }
                }
                // return success or not
                message_t m = buildSwitchResponse(msg.sender, success);
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
