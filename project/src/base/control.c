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
        message_t request = buildGetChildRequest(to_clone_pid);
        message_t response;
        sendMessage(&request);
        // Linka tutti i figli dell'hub clonato a sè stesso
        int child_pid;
        do {
            receiveMessage(&response);
            child_pid = response.vals[GET_CHILDREN_VAL_ID];
            if (child_pid != -1) {
                doLink(children, child_pid, getppid(), base_dir);
                message_t ack = buildResponse(to_clone_pid, -1);
                sendMessage(&ack);
            }
        } while (child_pid != -1);
        //  Invia la conferma al padre
        message_t confirm_clone = buildLinkResponse(getppid(), 1);
        sendMessage(&confirm_clone);
    }
    // Esecuzione control device
    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) {
            continue;
        } else {
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
                    int success = 1;
                    printf("TODO switch in control.c\n");
                    /*node_t *p = *children;
                    while (p != NULL) {
                        message_t m = buildSwitchRequest(msg.sender, success);
                        sendMessage(&m);
                        p = p->next;
                    }*/
                    message_t m = buildSwitchResponse(msg.sender, success);
                    sendMessage(&m);
                } break;

                case INFO_MSG_TYPE: {
                    // Stato = Override <-> lo stato dei componenti ad esso collegati non sono omogenei (intervento esterno all' HUB)
                    node_t *p = *children;
                    int count_on = 0, count_off = 0;
                    short override = 0;
                    while (p != NULL) {
                        message_t request = buildInfoRequest(p->value);
                        message_t response;
                        if (sendMessage(&request) == -1) {
                            perror("Error sending info request in control device");
                        } else if (receiveMessage(&response) == -1) {
                            perror("Error receiving info response in control device");
                        } else {
                            switch (response.vals[INFO_VAL_STATE]) {
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
                        }
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
                    listPrint(children);
                    message_t m = buildInfoResponseControl(msg.sender, children_str);  // Implementazione specifica dispositivo
                    m.vals[INFO_VAL_STATE] = children_state;
                    sendMessage(&m);
                } break;

                case LIST_MSG_TYPE: {
                    message_t m;
                    if (listEmpty(children)) {
                        m = buildListResponseControl(msg.sender, id, msg.vals[LIST_VAL_LEVEL], 1);  // Implementazione specifica dispositivo
                        sendMessage(&m);
                    } else {
                        m = buildListResponseControl(msg.sender, id, msg.vals[LIST_VAL_LEVEL], 0);  // Implementazione specifica dispositivo
                        sendMessage(&m);
                        doListControl(msg.sender, children);
                    }
                } break;

                case LINK_MSG_TYPE: {
                    doLink(children, msg.vals[LINK_VAL_PID], msg.sender, base_dir);
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
                        message_t response;
                        receiveMessage(&response);
                        p = p->next;
                    }
                    m = buildGetChildResponse(msg.sender, -1);
                    sendMessage(&m);
                } break;

                case DELETE_MSG_TYPE: {
                    signal(SIGCHLD, NULL);  // Rimuovo l'handler in modo da non interrompere l'esecuzione mentre leimino ricorsivamente i figli
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
    }
    return 0;
}

void sigchldHandler(int signum) {
    int pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) != -1) {
        listRemove(children, pid);
    }
}