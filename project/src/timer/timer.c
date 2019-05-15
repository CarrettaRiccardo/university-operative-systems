#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ipc.h"
#include "../include/list.h"
#include "../include/utils.h"

char *base_dir;
int id;
list_t child;  // Conterrà al max 1 figlio; uso una list_t perchè è compatibile con i metodi usati per hub

long begin = 0;          //momento temporale di attivazione
long end = 0;            //momento temporale di disattivazione
short waitForBegin = 0;  // 0 = prossimo evento è begin (NOW < begin), 1 = prossimo evento è end (begin < NOW < end)

void switchAlarm();
message_t buildInfoResponseTimer(int to_pid);
message_t buildListResponseTimer(int to_pid, int lv, short stop);

int main(int argc, char **argv) {
    base_dir = extractBaseDir(argv[0]);
    id = atoi(argv[1]);
    child = listInit();
    // associo il metodo switch al segnare alarm per il begin/end automatico
    signal(SIGALRM, switchAlarm);
    if (argc > 2) {  //  Clone del timer
        int to_clone_pid = atol(argv[2]);
        message_t request = buildGetChildRequest(to_clone_pid);
        message_t response;
        int child_pid;
        sendMessage(&request);
        // Linka il figlio del timer clonato a sè stesso
        receiveMessage(&response);
        child_pid = response.vals[GET_CHILDREN_VAL_ID];
        if (child_pid != -1) {
            doLink(child, child_pid, getppid(), base_dir);
            message_t ack = buildResponse(to_clone_pid, -1);
            sendMessage(&ack);
        }
        //  Invia la conferma al padre
        message_t confirm_clone = buildLinkResponse(getppid(), 1);
        sendMessage(&confirm_clone);
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
                int success = -1;
                if (msg.vals[SWITCH_VAL_POS] != __INT_MAX__) {  // se è un valore valido
                    switch (msg.vals[SWITCH_VAL_LABEL]) {       // set begin/end/stato del figlio
                        case LABEL_BEGIN_VALUE:
                            begin = msg.vals[SWITCH_VAL_POS];  // set begin
                            success = 1;
                            break;
                        case LABEL_END_VALUE:
                            end = msg.vals[SWITCH_VAL_POS];  // set end
                            success = 1;
                            break;
                        case LABEL_ALL_VALUE: /**/ success = 1; break;  // TODO
                    }
                    if (success == 1) {
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
                    }
                }
                // return success or not
                message_t m = buildSwitchResponse(msg.sender, success);
                sendMessage(&m);
            } else if (msg.type == LINK_MSG_TYPE) {
                if (listCount(child) < 1) {  // se non ha figlio
                    doLink(child, msg.vals[LINK_VAL_PID], msg.sender, base_dir);
                } else {  // non può avere più di un figlio
                    message_t fail = buildBusyResponse(msg.sender);
                    fail.vals[LINK_VAL_SUCCESS] = LINK_MAX_CHILD;
                    sendMessage(&fail);
                }
            } else if (msg.type == DELETE_MSG_TYPE) {
                message_t m = buildDeleteResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } else if (msg.type == TRANSLATE_MSG_TYPE) {
                message_t m = buildTranslateResponseControl(msg.sender, id, msg.vals[TRANSLATE_VAL_ID], child);
                sendMessage(&m);
            } else if (msg.type == LIST_MSG_TYPE) {  //  Risponde con i propri dati e inoltra la richiesta al figlio
                message_t m;
                if (listEmpty(child)) {
                    m = buildListResponseTimer(msg.sender, msg.vals[LIST_VAL_LEVEL], 1);
                    sendMessage(&m);
                } else {
                    m = buildListResponseTimer(msg.sender, msg.vals[LIST_VAL_LEVEL], 0);
                    sendMessage(&m);
                    doListControl(msg.sender, child);
                }
            } else if (msg.type == CLONE_MSG_TYPE) {
                int vals[NVAL] = {id, getpid()};
                message_t m = buildCloneResponse(msg.sender, TIMER, id, vals, 1);
                sendMessage(&m);
            } else if (msg.type == GET_CHILDREN_MSG_TYPE) {
                //  Invio il figlio al processo che lo richiede
                message_t m;
                node_t *p = *child;
                if (p != NULL) {
                    m = buildGetChildResponse(msg.sender, p->value);
                    sendMessage(&m);
                    message_t resp;
                    receiveMessage(&resp);
                }
                m = buildGetChildResponse(msg.sender, -1);
                sendMessage(&m);
            } else if (msg.type == DIE_MESG_TYPE) {
                //  Rimuovo il mittente di questo messaggio dalla lista del mio figlio
                listRemove(child, msg.sender);
            }
        }
    }

    return 0;
}

message_t buildInfoResponseTimer(int sender) {
    // Stato = Override <-> ??? -- TODO
    node_t *p = *child;
    int child_State = 0;  // 0 = figlio spento, 1 = figlio acceso
    short override = 0;   // 0 = no override, 1 = si override
    while (p != NULL) {
        message_t request = buildInfoRequest(p->value);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error sending info request in buildInfoResponseTimer");
        } else if (receiveMessage(&response) == -1) {
            perror("Error receiving info response in buildInfoResponseTimer");
        } else {
            if (response.type != INFO_MSG_TYPE) {
                message_t busy = buildBusyResponse(response.sender);
                sendMessage(&busy);
                continue;  // Faccio ripartire il ciclo sullo stesso figlio se leggo un messaggio non pertinente
            }
            child_State = response.vals[INFO_VAL_STATE];
        }
        p = NULL;
    }
    message_t ret = buildInfoResponse(sender);
    char *children_str;
    switch (child_State) {
        case 0: children_str = "off"; break;
        case 1: children_str = "on"; break;
        case 2: children_str = "off (override)"; break;
        case 3: children_str = "on (override)"; break;
    }

    sprintf(ret.text, "%s, state: %s, begin: %ld, end: %ld", TIMER, children_str, begin, end);
    ret.vals[INFO_VAL_STATE] = child_State;
    return ret;
}

void switchAlarm() {
    if (waitForBegin == 0) {  // da accendere (begin)
        // se ha figlio lo accendo
        if (listCount(child) == 1) {
            node_t *p = *child;
            if (p != NULL) {
                message_t m = buildSwitchRequest(p->value, LABEL_ALL_VALUE, SWITCH_POS_ON_VALUE);
                sendMessage(&m);
                message_t resp;
                receiveMessage(&resp);
            }
        }
        // setto il timer di spegnimento (end) se è maggiore dell'accensione (begin)
        if (time(NULL) < end) {
            alarm(end - time(NULL));
            waitForBegin = 1;
        } else {  // o termino gli alarm automatici
            alarm(0);
        }
    } else {  // da spegnere (end)
        if (waitForBegin == 1) {
            // se ha figlio lo spengo
            if (listCount(child) == 1) {
                node_t *p = *child;
                if (p != NULL) {
                    message_t m = buildSwitchRequest(p->value, LABEL_ALL_VALUE, SWITCH_POS_OFF_VALUE);
                    sendMessage(&m);
                    message_t resp;
                    receiveMessage(&resp);
                }
            }
            // setto il timer di accensione (begin) se è maggiore dello spegnimento (end)
            if (time(NULL) < begin) {
                alarm(begin - time(NULL));
                waitForBegin = 0;
            } else {  // o termino gli alarm automatici
                alarm(0);
            }
        }
    }
}
