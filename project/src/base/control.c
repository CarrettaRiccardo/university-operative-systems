#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/ipc.h"
#include "../include/list.h"
#include "../include/utils.h"

/* Metodi da implemantare nei dispositivi di controllo */
void initData();
void cloneData(char **vals);
message_t buildInfoResponseControl(int to_pid, char *children_state);
message_t buildListResponseControl(int to_pid, int id, int lv, short stop);
message_t buildCloneResponseControl(int to_pid, int id);

/* Gestione figlio eliminato */
void sigchldHandler(int signum);

char *base_dir;
int id;
list_t children;

int main(int argc, char **argv) {
    signal(SIGCHLD, sigchldHandler);

    base_dir = extractBaseDir(argv[0]);
    id = atoi(argv[1]);
    children = listInit();

    if (argc <= 2) {
        // Inizializzazione nuovo control device
        initData();
    } else {
        // Inzializzazione control device clonato
        cloneData(argv + 3);  // Salto i parametri [0] (percorso file), [1] (id) e [2] (to_clone_pid)
        // Clonazione ricorsiva dei figli
        int to_clone_pid = atol(argv[2]);
        // Linka tutti i figli dell'hub clonato a sè stesso
        message_t request = buildGetChildRequest(to_clone_pid);
        message_t response;
        sendMessage(&request);  // Richiedo tutti i figli di to_clone_pid
        int child_pid;
        do {
            receiveMessage(&response);
            child_pid = response.vals[GET_CHILDREN_VAL_ID];
            if (child_pid != -1) {
                doLink(children, child_pid, base_dir);  // Per ogni figlio di to_clone_pid, effettuo il link di quel device al nuovo device appena clonato
                message_t ack;
                receiveMessage(&ack);  //  Attendo una conferma di avvenuta clonazione dal figlio clonato
                ack.to = to_clone_pid;
                sendMessage(&ack);  // Richiedo prossimo figlio a to_clone_pid
            }
        } while (child_pid != -1);
        //  Invia la conferma al padre di avvenuta clonazione
        message_t confirm_clone = buildLinkResponse(getppid(), 1);
        sendMessage(&confirm_clone);
    }
    // Esecuzione control device
    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) continue;
        switch (msg.type) {
            case TRANSLATE_MSG_TYPE: {
                message_t m;
                if (id == msg.vals[TRANSLATE_VAL_ID]) {
                    m = buildTranslateResponse(msg.sender, getpid());
                } else {
                    // Inoltro la richiesta ai figli
                    int to_pid = getPidById(children, msg.vals[TRANSLATE_VAL_ID]);
                    m = buildTranslateResponse(msg.sender, to_pid);
                }
                sendMessage(&m);
            } break;

            case SWITCH_MSG_TYPE: {
                int success = -1;
                if (msg.vals[SWITCH_VAL_POS] != __INT_MAX__) {  // se è un valore valido
                    switch (msg.vals[SWITCH_VAL_LABEL]) {
                        /*case LABEL_BEGIN_VALUE:
                            begin = msg.vals[SWITCH_VAL_POS];  // set begin
                            success = 1;
                            break;
                        case LABEL_END_VALUE:
                            end = msg.vals[SWITCH_VAL_POS];  // set end
                            success = 1;
                            break;*/
                        case LABEL_ALL_VALUE: 
                            // se ha un figlio, fa lo switch generico di esso
                            if (listCount(children) > 0){
                                node_t *p = *children;
                                while (p != NULL) {
                                    message_t m = buildSwitchRequest(p->value, LABEL_ALL_VALUE, msg.vals[SWITCH_VAL_POS]);
                                    sendMessage(&m);
                                    message_t resp;
                                    receiveMessage(&resp);
                                    p = p->next;
                                }
                            }
                            success = 1;
                            break;
                    }
                    // TODO spostare in comando apposito
                    /*if (success == 1) {
                        // faccio partire il primo evento automatico (sovrascriverà la precendente alarm(..), ma il tempo di attesa rimanente è ricalcolato)
                        if (begin > time(NULL)) {
                            if (begin < end || (begin >= end && end <= time(NULL))) {  // attendo il begin
                                alarm(begin - time(NULL));
                                waitForBegin = 0;
                            } else {  // attendo l'end
                                alarm(end - time(NULL));
                                waitForBegin = 1;
                            }
                        } else {
                            if (end > time(NULL)) {  // attendo l'end
                                alarm(end - time(NULL));
                                waitForBegin = 1;
                            }
                            // altrimenti sono tutti e due eventi passati
                        }
                    }*/
                }
                // return success or not
                message_t m = buildSwitchResponse(msg.sender, success);
                sendMessage(&m);
            } break;

            case INFO_MSG_TYPE: {
                doInfoControl(msg.sender, children);

            } break;

            case LIST_MSG_TYPE: {
                message_t m;
                if (listEmpty(children)) {
                    m = buildListResponseControl(msg.sender, id, msg.vals[LIST_VAL_LEVEL], 1);  // Implementazione specifica dispositivo
                    sendMessage(&m);
                } else {
                    m = buildListResponseControl(msg.sender, id, msg.vals[LIST_VAL_LEVEL], 0);  // Implementazione specifica dispositivo
                    sendMessage(&m);
                    //  Inoltro richiesta LIST ai figli
                    node_t *p = *children;
                    while (p != NULL) {
                        int son = p->value;
                        message_t request = buildListRequest(son);
                        if (sendMessage(&request) == -1)
                            printf("Error sending list control request to pid %d: %s\n", son, strerror(errno));

                        message_t response;
                        int stop = 0;
                        do {
                            // TODO: implementare BUSY globalmente
                            do {  // Se ricevo un messaggio diverso da quello che mi aspetto, rispondo BUSY
                                if (receiveMessage(&response) == -1)
                                    perror("Error receiving list control response");
                                if (response.type != LIST_MSG_TYPE) {
                                    message_t busy = buildBusyResponse(response.sender);
                                    sendMessage(&busy);
                                }
                            } while (response.type != LIST_MSG_TYPE);

                            response.to = getppid();             // Cambio il destinatario per farlo arrivare a mio padre 
                            response.vals[LIST_VAL_LEVEL] += 1;  //  Aumento il valore "livello"
                            stop = response.vals[LIST_VAL_STOP];
                            response.vals[LIST_VAL_STOP] = 0;  //  Tolgo lo stop dalla risposta
                            if (stop == 1 && p->next == NULL) {
                                //  Ultimo figlio, imposto lo stop
                                response.vals[LIST_VAL_STOP] = 1;
                            }
                            sendMessage(&response);
                        } while (stop != 1);
                        p = p->next;
                    }
                }
            } break;

            case LINK_MSG_TYPE: {
                doLink(children, msg.vals[LINK_VAL_PID], base_dir);
                //  Attendo una conferma dal figlio clonato e la inoltro al padre.
                message_t ack;
                receiveMessage(&ack);
                ack.sender = getpid();
                ack.to = msg.sender;
                sendMessage(&ack);
            } break;

            case CLONE_MSG_TYPE: {
                message_t m = buildCloneResponseControl(msg.sender, id);  // Implementazione specifica dispositivo
                sendMessage(&m);
            } break;

            case GET_CHILDREN_MSG_TYPE: {
                //  Invio tutti i figli al processo che lo richiede
                message_t m;
                node_t *p = *children;
                while (p != NULL) {
                    m = buildGetChildResponse(msg.sender, p->value);
                    sendMessage(&m);
                    receiveMessage(NULL);  // ACK, conferma ricezione dati figlio
                    p = p->next;
                }
                m = buildGetChildResponse(msg.sender, -1);
                sendMessage(&m);
            } break;

            case DELETE_MSG_TYPE: {
                signal(SIGCHLD, NULL);  // Rimuovo l'handler in modo da non interrompere l'esecuzione mentre elimino ricorsivamente i figli
                node_t *p = *children;
                message_t kill_req, kill_resp;
                while (p != NULL) {
                    kill_req = buildDeleteRequest(p->value);
                    sendMessage(&kill_req);
                    receiveMessage(&kill_resp);
                    p = p->next;
                }
                message_t m = buildDeleteResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } break;
        }
    }
    return 0;
}

void sigchldHandler(int signum) {
    int pid;
    do {
        pid = waitpid(-1, NULL, WNOHANG);
        listRemove(children, pid);
    } while (pid != -1 && pid != 0);
}

void doInfoControl(int to_pid) {
    // Stato = Override <-> lo stato dei componenti ad esso collegati non sono omogenei (intervento esterno all' HUB)
    list_msg_t messaggi = listInit();
    node_t *p = *children;
    int count_on = 0, count_off = 0;
    short override = 0;
    while (p != NULL) {
        message_t request = buildInfoRequest(p->value);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error sending info request in control device");
        }

        int stop = 0;
        do {
            do {  // Se ricevo un messaggio diverso da quello che mi aspetto, rispondo BUSY
                if (receiveMessage(&response) == -1)
                    perror("Error receiving list control response");
                if (response.type != INFO_MSG_TYPE) {
                    message_t busy = buildBusyResponse(response.sender);
                    sendMessage(&busy);
                }
            } while (response.type != INFO_MSG_TYPE);

            switch (response.vals[INFO_VAL_STATE]) {  //capisco se c'è stato un override rispetto l'HUB
                case 0: count_off++; break;
                case 1: count_on++; break;
                case 2:
                    count_off++;
                    override = 1;
                    break;
                case 3:
                    count_on++;
                    override = 1;
                    break;
            }

            stop = response.vals[INFO_VAL_STOP];
            response.to = to_pid;                // Cambio il destinatario per farlo arrivare a mio padre
            response.vals[INFO_VAL_LEVEL] += 1;  //  Aumento il valore "livello" per identazione
            response.vals[INFO_VAL_STOP] = 0;    //  Tolgo lo stop dalla risposta
            listMsgPush(&messaggi, response);
        } while (stop != 1);

        p = p->next;
    }
    short children_state;
    if (override == 0 && count_on == 0)
        children_state = 0;  // off
    else if (override == 0 && count_off == 0)
        children_state = 1;  // on
    else
        children_state = (count_off >= count_on) ? 2 : 3;  // off (override) / on (override)
    char *children_str;
    switch (children_state) {
        case 0: children_str = "off"; break;
        case 1: children_str = "on"; break;
        case 2: children_str = "off (override)"; break;
        case 3: children_str = "on (override)"; break;
    }
    message_t m = buildInfoResponseControl(to_pid, children_str);  // Implementazione specifica dispositivo
    m.vals[INFO_VAL_STATE] = children_state;
    m.vals[INFO_VAL_STOP] = 0;
    sendMessage(&m);

    /**p = *messaggi;
    while (p != NULL) {
        message_t response = buildResponse(p->value, );
        if (sendMessage(&response) == -1) {
            perror("Error sending info request in control device");
        }
        p = p->next;
    }*/
    freeMsgList(&messaggi);
}

//Implementa il metodo LIST per un dispositivo di Controllo (Hub o Timer)
void doListControl(int to_pid, list_t children) {
}