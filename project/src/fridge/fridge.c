/*
TODO: Remove not-allowed libraries
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"

int main(int argc, char **argv) {
    const int id = atoi(argv[1]);
    const int mqid = getMq();                 //Ottengo accesso per la MailBox
    const int sessione = (int)atoi(argv[1]);  //Ottengo il valore di sessione passato da mio padre

    short stato = 0;         //0 = chiusa, 1 = aperta
    short interruttore = 0;  //0 = fermo, 1 = apertura/chiusura
    short delay = 0;         // tempo di chiusura automatica porta
    short temperatura = 0;   // temperatura interna
    short perc = 0;          // percentuale riempimento (0-100%)
    unsigned long last_open_time = 0; // time ultima apertura
    unsigned int tempo;      //tempo apertura porta

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
            // ritorna info
        } else if (msg.type == SWITCH_MSG_TYPE) {
                int success = -1;
                if (msg.vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE) {   // interruttore (apri/chiudi)
                    if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {  // chiudo
                        // controllo se il tempo di chiusura automatica NON è superato
                        if (stato == SWITCH_POS_ON_VALUE && last_open_time + time(NULL) < last_open_time + delay) {
                            // se sì, sommo la differenza di tempo attuale al tempo di apertura...
                            tempo += time(NULL) - last_open_time;
                            // ...e chiudo la porta
                            stato = SWITCH_POS_OFF_VALUE;
                        }
                        // altrimenti è gia su "off"
                        success = 1;
                    }
                    if (msg.vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {  // apro
                        stato = SWITCH_POS_ON_VALUE;// TODO
                        success = 1;
                    }
                } else { if (msg.vals[SWITCH_VAL_LABEL] == LABEL_TERM_VALUE) {   // termostato
                    if (msg.vals[SWITCH_VAL_POS] != -1) {  // cambio valore
                        temperatura = (short)msg.vals[1];
                        success = 1;
                        tempo += time(NULL) - last_open_time;
                    }
                }
            }
            // return success or not
            message_t m = buildSwitchResponse(success, msg.sender);
            sendMessage(&m);
        }
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
        else if (msg.type == LINK_MSG_TYPE) {
            // link
            // (value = id a cui linkare)
        } else if (msg.type == DELETE_MSG_TYPE) {
            exit(0);
        } else if (msg.type == TRANSLATE_MSG_TYPE) {
            //Message m = buildTranslateResponse(id, sessione, msg.value, msg.sender);
            //sendMessage(mqid, m);
        }
    }

    return 0;
}
