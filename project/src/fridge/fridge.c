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
    unsigned int tempo;      //tempo apertura porta

    if (sessione == 0) {                       //Sessione diversa da quella corrente.
        printf("Errore sessione reader = 0");  //TODO: Gestire correttamente la morte  del processo
        exit(1);
    }

    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) continue;  //messaggio da ignorare (per sessione diversa/altri casi)

        if (strcmp(msg.text, "ECHO") == 0) {
            //Message m = {.to = getppid(), .session = sessione, .value = tempo, .state = stato};
            //sendMessage( mqid, m );
        } else if (strcmp(msg.text, INFO_REQUEST) == 0) {
            // ritorna info
        } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
            // apertura/chiusura
        } else if (strcmp(msg.text, SET_TIME_DELAY) == 0) {
            // delay chiusura porta
        } else if (strcmp(msg.text, SET_TEMPERATURE) == 0) {
            // temperatura interna
        } else if (strcmp(msg.text, SET_PERC_FILLED) == 0) {  // solo da esterno
            // percentuale riempimento
        } else if (strcmp(msg.text, MSG_LINK) == 0) {
            // link
            // (value = id a cui linkare)
        } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
            // switch
            // da gestire
        } else if (strcmp(msg.text, MSG_DELETE_REQUEST) == 0) {
            exit(0);
        } else if (strcmp(msg.text, MSG_TRANSLATE) == 0) {
            //Message m = buildTranslateResponse(id, sessione, msg.value, msg.sender);
            //sendMessage(mqid, m);
        }
    }

    return 0;
}
