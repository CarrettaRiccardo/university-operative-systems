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

int id;
short state;
short interruttore;  // valore interruttore che è 1 a 1 con lo stato
unsigned long long on_time;
unsigned long long last_start_time;
unsigned long long controller_pid;  //Aggiunto da Steve in forma temporanea

/* Override specifico per il metodo definito in IPC */
message_t buildInfoResponseBulb(int to_pid);
message_t buildListResponseBulb(int to_pid, int lv);

int main(int argc, char **argv) {
    id = atoi(argv[1]);  // Lettura id da parametro
    //  Creazione nuova bulb
    if (argc <= 2) {
        state = SWITCH_POS_OFF_VALUE;  // 0 = spenta, 1 = accesa
        interruttore = state;
        on_time = 0;                   //TODO: Destro fare lettura on_time da parametro in caso di clonazione
        last_start_time = time(NULL);  // Tempo accensione lampadina
    }
    //  Inzializzazione parametri da richiesta clone
    else {
        state = atoi(argv[2]);
        interruttore = state;
        on_time = atoi(argv[3]);
        last_start_time = atoi(argv[4]);
        //  Invia la conferma al padre
        message_t confirm_clone = buildLinkResponse(getppid(), 1);
        sendMessage(&confirm_clone);
    }
    while (1) {
        message_t msg;
        int result = receiveMessage(&msg);
        if (result == -1) {
            perror("BULB: Error receive message");
        } else {
            if (msg.to == -1) continue;  // Messaggio da ignorare (per sessione diversa/altri casi)

            if (msg.type == DELETE_MSG_TYPE) {  //TODO: Gestire il caso del DELETE da override, che porta ad un problema percheè sender!=ppi ma il controller non sta in ascolto di messaggi
                message_t m = buildDeleteResponse(msg.sender);
                sendMessage(&m);
                if (getppid() != msg.sender && getppid() != controller_pid) {  //sto morendo, invio conferma di ricezione al mittente, e nel caso che il mittente non sia mio padre, invio un messaggio a mio padre di rimuovermi dalla lista dei suoi figli
                    message_t m = buildDieMessage(getppid());
                    sendMessage(&m);
                }
                exit(0);
            } else if (msg.type == INFO_MSG_TYPE) {
                message_t m = buildInfoResponseBulb(msg.sender);
                sendMessage(&m);
            } else if (msg.type == SWITCH_MSG_TYPE) {
                int success = -1;
                if (msg.vals[SWITCH_VAL_LABEL] == LABEL_LIGHT_VALUE || msg.vals[SWITCH_VAL_LABEL] == LABEL_GENERIC_SWITCH_VALUE) {  // interruttore (luce) o generico (da hub ai propri figli)
                    if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {                                                         // spengo
                        // se è accesa, sommo il tempo di accensione e spengo
                        if (interruttore == SWITCH_POS_ON_VALUE) {
                            on_time += time(NULL) - last_start_time;
                            interruttore = SWITCH_POS_OFF_VALUE;
                            state = interruttore;
                        }
                        success = 1;
                    }
                    if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {  // accendo
                        // se è spenta, accendo e salvo il tempo di accensione
                        if (interruttore == SWITCH_POS_OFF_VALUE) {
                            last_start_time = time(NULL);
                            interruttore = SWITCH_POS_ON_VALUE;
                            state = interruttore;
                        }
                        success = 1;
                    }
                }
                // return success or not
                message_t m = buildSwitchResponse(msg.sender, success);
                sendMessage(&m);
            } else if (msg.type == TRANSLATE_MSG_TYPE) {
                message_t m = buildTranslateResponse(msg.sender, msg.vals[TRANSLATE_VAL_ID] == id ? getpid() : -1);
                sendMessage(&m);
            } else if (msg.type == LIST_MSG_TYPE) {  // Caso base per la LIST. value5 = 1 per indicare fine albero
                message_t m = buildListResponseBulb(msg.sender, msg.vals[LIST_VAL_LEVEL]);
                sendMessage(&m);
            } else if (msg.type == LINK_MSG_TYPE) {
                message_t m = buildLinkResponse(msg.sender, -1);
                sendMessage(&m);
            } else if (msg.type == CLONE_MSG_TYPE) {
                int vals[NVAL] = {id, state, on_time, last_start_time};
                message_t m = buildCloneResponse(msg.sender, BULB, vals);
                sendMessage(&m);
            }
        }
    }
    return 0;
}

message_t buildInfoResponseBulb(int to_pid) {
    message_t ret = buildInfoResponse(to_pid);
    time_t now = time(NULL);
    unsigned long long work_time = on_time + (now - ((state == SWITCH_POS_OFF_VALUE) ? now : last_start_time));  // Se è spenta ritorno solo "on_time", altrimenti on_time+differenza da quanto accesa
    sprintf(ret.text, "%s, state: %s, registers: time=%llds", BULB, state == 1 ? "on" : "off", work_time);
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildListResponseBulb(int to_pid, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, "%s %s", BULB, state == 1 ? "on" : "off");
    return ret;
}