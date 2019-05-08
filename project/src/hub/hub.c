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
    const int mqid = getMq();                 // Ottengo accesso per la MailBox
    const int sessione = (int)atoi(argv[1]);  // Ottengo il valore di sessione passato da mio padre

    while (1) {
        message_t msg;

        if (receiveMessage(getpid(), &msg) == -1) continue;  // Messaggio da ignorare (per sessione diversa/altri casi)

        if (strcmp(msg.text, INFO_REQUEST) == 0) {
            // ritorna info
        } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
            // apertura/chiusura
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
