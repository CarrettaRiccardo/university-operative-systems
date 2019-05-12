/*
TODO: Remove not-allowed libraries
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"

int id;
int mqid;

short state;
short interruttore;
int delay;
int temp;
int perc;
unsigned long long last_open_time;
unsigned long long open_time;

message_t buildInfoResponseFridge(int to_pid);
message_t buildListResponseFridge(int to_pid, int lv);

int main(int argc, char **argv) {
    id = atoi(argv[1]);
    mqid = getMq();  //Ottengo accesso per la MailBox

    state = SWITCH_POS_OFF_VALUE;         //0 = chiusa, 1 = aperta
    interruttore = SWITCH_POS_OFF_VALUE;  //0 = fermo, 1 = apertura/chiusura (torna subito ad off, ma se azionato apre la porta o la chiude)
    delay = 13;                           // tempo di chiusura automatica porta
    temp = 4;                             // temperatura interna
    perc = 22;                            // percentuale riempimento (0-100%)
    last_open_time = 0;                   // time ultima apertura
    open_time = 0;                        //tempo apertura porta

    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) continue;  //messaggio da ignorare (per sessione diversa/altri casi)

        /* UPDATE: ad ogni ricezione di messaggio, aggiorno le proprietà del fridge */
        // controllo se il tempo di chiusura automatica è superato
        if (state == SWITCH_POS_ON_VALUE && last_open_time + delay <= time(NULL)) {
            // se sì, sommo il "delay" al tempo di apertura...
            open_time += delay;
            // ...e chiudo automaticamente la porta
            state = SWITCH_POS_OFF_VALUE;
        }

        if (msg.type == INFO_MSG_TYPE) {
            message_t m = buildInfoResponseFridge(msg.sender);
            sendMessage(&m);
        } else if (msg.type == SWITCH_MSG_TYPE) {
            int success = -1;
            if (msg.vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE || msg.vals[SWITCH_VAL_LABEL] == LABEL_GENERIC_SWITCH_VALUE) {  // interruttore (apri/chiudi) o generico (da hub ai propri figli)
                if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {                                                        // chiudo
                    // controllo se il tempo di chiusura automatica NON è superato
                    if (state == SWITCH_POS_ON_VALUE && last_open_time + delay > time(NULL)) {
                        // se non lo è (quindi deve ancora chiudersi automaticamente), sommo la differenza di tempo attuale al tempo di apertura...
                        open_time += time(NULL) - last_open_time;
                        // ...e chiudo la porta
                        state = SWITCH_POS_OFF_VALUE;
                        interruttore = SWITCH_POS_OFF_VALUE;
                    }
                    // altrimenti è gia su "off"
                    success = 1;
                }
                if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {  // apro
                    // se è chiuso
                    if (state == SWITCH_POS_OFF_VALUE) {
                        // apro la porta e salvo il tempo di apertura
                        last_open_time = time(NULL);
                        state = SWITCH_POS_ON_VALUE;
                        interruttore = SWITCH_POS_OFF_VALUE;
                    }
                    // altrimenti è gia su "on"
                    success = 1;
                }
            } else {
                if (msg.vals[SWITCH_VAL_LABEL] == LABEL_TERM_VALUE) {  // termostato
                    if (msg.vals[SWITCH_VAL_POS] != __LONG_MAX__) {    // cambio valore
                        temp = msg.vals[SWITCH_VAL_POS];
                        success = 1;
                        open_time += time(NULL) - last_open_time;
                    }
                }
            }
            // return success or not
            message_t m = buildSwitchResponse(msg.sender, success);
            sendMessage(&m);

            /*
                TODO: Questi comandi vanno fatti sempre con lo switch
                else if (strcmp(msg.text, SET_TIME_DELAY) == 0) {
                    // delay chiusura porta
                } else if (strcmp(msg.text, SET_TEMPERATURE) == 0) {
                    // temperatura interna
                } else if (strcmp(msg.text, SET_PERC_FILLED) == 0) {  // solo da esterno
                    // percentuale riempimento
                } 
            */
        } else if (msg.type == LINK_MSG_TYPE) {
            // link
            // (value = id a cui linkare)
        } else if (msg.type == DELETE_MSG_TYPE) {
            message_t m = buildDeleteResponse(msg.sender);
            sendMessage(&m);
            if (getppid() != msg.sender) {  //sto morendo, invio conferma di ricezione al mittente, e nel caso che il mittente non sia mio padre, invio un messaggio a mio padre di rimuovermi dalla lista dei suoi figli
                message_t m = buildDieMessage(getppid());
                sendMessage(&m);
            }
            exit(0);
        } else if (msg.type == TRANSLATE_MSG_TYPE) {
            message_t m = buildTranslateResponse(msg.sender, msg.vals[TRANSLATE_VAL_ID] == id ? getpid() : -1);
            sendMessage(&m);
        } else if (msg.type == LIST_MSG_TYPE) {
            message_t m = buildListResponseFridge(msg.sender, msg.vals[LIST_VAL_LEVEL]);
            sendMessage(&m);
        }
    }

    return 0;
}

message_t buildInfoResponseFridge(int to_pid) {
    message_t ret = buildInfoResponse(to_pid);
    time_t now = time(NULL);
    unsigned long long open_time = open_time + (now - ((state == 0) ? now : last_open_time));  //se è chiusa ritorno solo "tempo", altrimenti tempo+differenza da quanto accesa
    sprintf(ret.text, "%s, state: %s, registers: time=%llds delay=%d perc=%d%% temp=%d°C", FRIDGE, state == 1 ? "open" : "closed", open_time, delay, perc, temp);
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildListResponseFridge(int to_pid, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, "%s %s", FRIDGE, state == 1 ? "open" : "closed");
    return ret;
}