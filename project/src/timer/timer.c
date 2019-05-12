#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"
#include "../include/list.h"
#include "../include/utils.h"

char *base_dir;
int id;
list_t children;

long begin = 0;  //momento temporale di attivazione
long end = 0;    //momento temporale di disattivazione

//Override del metodo in IPC.C per il componente Timer
message_t buildInfoResponseTimer(int sender);

int main(int argc, char **argv) {
    base_dir = extractBaseDir(argv[0]);
    id = atoi(argv[1]);
    children = listInit();
    if (argc > 2) {  //  Clone del timer
        int to_clone_pid = atol(argv[2]);
        message_t request = buildGetChildRequest(to_clone_pid);
        message_t response;
        int child_pid;
        sendMessage(&request);
        // Linka tutti il figlio del timer clonato a sè stesso
        receiveMessage(&response);
        child_pid = response.vals[GET_CHILDREN_VAL_ID];
        if (child_pid != -1) {
            doLink(children, child_pid, getppid(), base_dir);
            message_t ack = buildResponse(to_clone_pid, -1);
            sendMessage(&ack);
        }
    }

    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) {
            perror("TIMER: Error receive message");
        } else {
            if (msg.type == INFO_MSG_TYPE) {
                message_t m = buildInfoResponseTimer(msg.sender);
                sendMessage(&m);
            } else if (msg.type == SWITCH_MSG_TYPE) {
                // apertura/chiusura
            } else if (msg.type == LINK_MSG_TYPE) {
                if (listCount(children) < 1)// se non ha figlio
                    doLink(children, msg.vals[LINK_VAL_PID], msg.sender, base_dir);
                else
                    continue;// TODO
            } else if (msg.type == DELETE_MSG_TYPE) {
                message_t m = buildDeleteResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } else if (msg.type == TRANSLATE_MSG_TYPE) {
                message_t m = buildTranslateResponseControl(msg.sender, id, msg.vals[TRANSLATE_VAL_ID], children);
                sendMessage(&m);
            } else if (msg.type == LIST_MSG_TYPE) {  //  Risponde con i propri dati e inoltra la richiesta al figlio
                message_t m;
                if (listEmpty(children)) {
                    m = buildListResponse(msg.sender, id, TIMER, msg.vals[LIST_VAL_LEVEL], 1);
                    sendMessage(&m);
                } else {
                    m = buildListResponse(msg.sender, id, TIMER, msg.vals[LIST_VAL_LEVEL], 0);
                    sendMessage(&m);
                    doListControl(msg.sender, children);
                }
            } else if (msg.type == CLONE_MSG_TYPE) {
                int vals[NVAL] = {id, getpid()};
                message_t m = buildCloneResponse(msg.sender, TIMER, vals);
                sendMessage(&m);
            } else if (msg.type == GET_CHILDREN_MSG_TYPE) {
                //  Invio il figlio al processo che lo richiede
                message_t m;
                node_t *p = *children;
                if (p != NULL) {
                    m = buildGetChildResponse(msg.sender, p->value);
                    sendMessage(&m);
                    message_t resp;
                    receiveMessage(&resp);
                }
                m = buildGetChildResponse(msg.sender, -1);
                sendMessage(&m);
            } else if (msg.type == DIE_MESG_TYPE) {
                //  Rimuovo il mittente di questo messaggio dalla lista dei miei figli
                listRemove(children, msg.sender);
            }
        }
    }

    return 0;
}

//Stato = Override <-> è presente nel TIMER ?? --  TODO
message_t buildInfoResponseTimer(int sender) {
    node_t *p = *children;
    short stato_figlio = -1;

    while (p != NULL) {
        int id_processo = p->value;
        message_t request = buildInfoRequest(id_processo);
        message_t response;
        
        if (sendMessage(&request) == -1) {
            perror("Error get pid by id request");
        } else if (receiveMessage(&response) == -1) {
            perror("Error get pid by id response");
        } else {
            if(response.type != INFO_MSG_TYPE){
                message_t busy = buildBusyResponse(response.sender);
                sendMessage(&busy);
                continue;//per evitare annidamento di if e parentesi, faccio ripartire il ciclo sullo stesso figlio. Letto un messaggio non pertinente
            }

            if(stato_figlio != -1 && stato_figlio != response.vals[INFO_VAL_STATE]){
                message_t ret = buildInfoResponse(sender,TIMER);
                ret.vals[INFO_VAL_STATE] = 3; //stato di override
                return ret;
            }
            else{
                stato_figlio = response.vals[INFO_VAL_STATE];
            }
        }
        // fa terminare il ciclo (saltato in caso legga un messaggio errato per il figlio)
        p = NULL;
    }
    message_t ret = buildInfoResponse(sender, TIMER);
    ret.vals[INFO_VAL_STATE] = stato_figlio; //stato di override
    return ret;
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