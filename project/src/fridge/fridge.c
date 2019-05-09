/*
TODO: Remove not-allowed libraries
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"

/* Override specifico per il metodo definito in IPC */
message_t buildInfoResponseFridge(long to_pid, short state, long open_time, short del, short temp, short percent);

int main(int argc, char **argv) {
    const int id = atoi(argv[1]);
    const int mqid = getMq();                 //Ottengo accesso per la MailBox
    const int sessione = (int)atoi(argv[1]);  //Ottengo il valore di sessione passato da mio padre

    short stato = 0;         //0 = chiusa, 1 = aperta
    short interruttore = 0;  //0 = fermo, 1 = apertura/chiusura (torna subito ad off, ma se azionato apre la porta o la chiude)
    short delay = 0;         // tempo di chiusura automatica porta
    short temperatura = 0;   // temperatura interna
    short perc = 0;          // percentuale riempimento (0-100%)
    unsigned long last_open_time = 0; // time ultima apertura
    unsigned int tempo = 0;      //tempo apertura porta

    if (sessione == 0) {                       //Sessione diversa da quella corrente.
        printf("Errore sessione reader = 0");  //TODO: Gestire correttamente la morte  del processo
        exit(1);
    }

    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) continue;  //messaggio da ignorare (per sessione diversa/altri casi)

        /* UPDATE: ad ogni ricezione di messaggio, aggiorno le proprietà del fridge */
        // controllo se il tempo di chiusura automatica è superato
        if (stato == SWITCH_POS_ON_VALUE && last_open_time + delay <= time(NULL)){
            // se sì, sommo il "delay" al tempo di apertura...
            tempo += delay;
            // ...e chiudo automaticamente la porta
            stato = SWITCH_POS_OFF_VALUE;
        }


        if (strcmp(msg.text, "ECHO") == 0) {
            //Message m = {.to = getppid(), .session = sessione, .value = tempo, .state = stato};
            //sendMessage( mqid, m );
        } else if (msg.type == INFO_MSG_TYPE) {
            time_t now = time(NULL);
            unsigned long open_time = tempo + (now - ((stato == 0) ? now : last_open_time));  //se è chiusa ritorno solo "tempo", altrimenti tempo+differenza da quanto accesa
            message_t m = buildInfoResponseFridge(msg.sender, stato, open_time, delay, temperatura, perc);
            sendMessage(&m);
        } else if (msg.type == SWITCH_MSG_TYPE) {
            int success = -1;
            if (msg.vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE || msg.vals[SWITCH_VAL_LABEL] == LABEL_GENERIC_SWITCH_VALUE) { // interruttore (apri/chiudi) o generico (da hub ai propri figli)
                if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {  // chiudo
                    // controllo se il tempo di chiusura automatica NON è superato
                    if (stato == SWITCH_POS_ON_VALUE && last_open_time + delay > time(NULL)) {
                        // se non lo è (quindi deve ancora chiudersi automaticamente), sommo la differenza di tempo attuale al tempo di apertura...
                        tempo += time(NULL) - last_open_time;
                        // ...e chiudo la porta
                        stato = SWITCH_POS_OFF_VALUE;
                        interruttore = SWITCH_POS_OFF_VALUE;
                    }
                    // altrimenti è gia su "off"
                    success = 1;
                }
                if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {  // apro
                    // se è chiuso
                    if (stato == SWITCH_POS_OFF_VALUE) {
                        // apro la porta e salvo il tempo di apertura
                        last_open_time = time(NULL);
                        stato = SWITCH_POS_ON_VALUE;
                        interruttore = SWITCH_POS_OFF_VALUE;
                    }
                    // altrimenti è gia su "on"
                    success = 1;
                }
            } else { 
                if (msg.vals[SWITCH_VAL_LABEL] == LABEL_TERM_VALUE) {   // termostato
                    if (msg.vals[SWITCH_VAL_POS] != __LONG_MAX__) {  // cambio valore
                        temperatura = (short)msg.vals[SWITCH_VAL_POS];
                        success = 1;
                        tempo += time(NULL) - last_open_time;
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
            exit(0);
        } else if (msg.type == TRANSLATE_MSG_TYPE) {
            message_t m = buildTranslateResponse(msg.sender, msg.vals[TRANSLATE_VAL_ID] == id ? 1 : 0);
            sendMessage(&m);
        } else if (msg.type == LIST_MSG_TYPE) {
            message_t m = buildListResponse(msg.sender, FRIDGE, id, msg.vals[0], stato, 1);
            sendMessage(&m);
        }
    }

    return 0;
}

message_t buildInfoResponseFridge(long to_pid, short state, long open_time, short del, short temp, short percent) {
    message_t ret = buildInfoResponse(to_pid, FRIDGE);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[1] = open_time;
    ret.vals[2] = del;
    ret.vals[3] = temp;
    ret.vals[4] = percent;
    return ret;
}
