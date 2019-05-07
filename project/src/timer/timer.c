/*
TODO: Remove not-allowed libraries
*/

int main(int argc, char **argv) {
    return 0;
}

/*
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"

int main(int argc, char **argv) {
    const int id = 7; //TODO: Leggere id da parametro
    const int mqid = getMq();  //Ottengo accesso per la MailBox
    const int sessione = (int)atoi(argv[1]);  //Ottengo il valore di sessione passato da mio padre

    int tempo,stato; //TODO: Inserite solo per evitare errori di compilazione

    // TODO
    //short begin = 0;  //momento temporale di attivazione
    //short end = 0;  //momento temporale di disattivazione

    if(sessione == 0){ //Sessione diversa da quella corrente. 
        printf("Errore sessione reader = 0"); //TODO: Gestire correttamente la morte  del processo
        exit(1);
    }

    while (1){
        message_t msg = receiveMessage(getpid());

        if(msg.to == -1) //messaggio da ignorare (per sessione diversa/altri casi)
            continue;

        if( strcmp(msg.text, "ECHO") == 0 ){
            message_t m = {.to = getppid(), .session = sessione, .value = tempo, .state = stato};
            sendMessage( m );
        }
        else if (strcmp(msg.text, INFO_REQUEST) == 0){
            message_t m = buildInfoResponse(id,  tempo, stato, msg.sender, "Timer");
            sendMessage(m);
        }
        else if (strcmp(msg.text, SWITCH_ON) == 0){
            // accensione figlio
        }
        else if (strcmp(msg.text, SWITCH_OFF) == 0){
            // spegnimento figlio
        }
        else if (strcmp(msg.text, SET_TIME_BEGIN) == 0){
            // tempo di attivazione automatica
        }
        else if (strcmp(msg.text, SET_TIME_END) == 0){
            // tempo di disattivazione automatica
        }
        else if (strcmp(msg.text, MSG_LINK) == 0){
            // link
            // (value = id a cui linkare)
        }
        else if (strcmp(msg.text, MSG_SWITCH) == 0){
            // switch
            // da gestire
        }
        else if (strcmp(msg.text, MSG_DELETE_REQUEST) == 0){
            exit(0);
        }
        else if (strcmp(msg.text, MSG_TRANSLATE) == 0){
            message_t m = buildTranslateResponse(id, msg.value, msg.sender);
            sendMessage(m);
        }
    }

    return 0;
}


*/