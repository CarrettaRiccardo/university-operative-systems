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

message_t buildInfoResponseHub(int to_pid);
message_t buildListResponseHub(int to_pid, int lv, short stop);
void clone(char **argv);

int main(int argc, char **argv) {
    base_dir = extractBaseDir(argv[0]);
    id = atoi(argv[1]);
    children = listInit();

    if (argc > 2) {
        // Copia dati da argv per clonazione. Usato nel comando link
        clone(argv);
    }

    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) {
            perror("HUB: Error receive message");
        } else {
            if (msg.type == INFO_MSG_TYPE) {
                message_t m = buildInfoResponseHub(msg.sender);
                sendMessage(&m);
            } else if (msg.type == SWITCH_MSG_TYPE) {
                // apertura/chiusura
            } else if (msg.type == LINK_MSG_TYPE) {
                doLink(children, msg.vals[LINK_VAL_PID], msg.sender, base_dir);
            } else if (msg.type == DELETE_MSG_TYPE) {
                message_t m = buildDeleteResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } else if (msg.type == TRANSLATE_MSG_TYPE) {
                message_t m = buildTranslateResponseControl(msg.sender, id, msg.vals[TRANSLATE_VAL_ID], children);
                sendMessage(&m);
            } else if (msg.type == LIST_MSG_TYPE) {  //  Risponde con i propri dati e inoltra la richiesta ai figli
                message_t m;
                if (listEmpty(children)) {
                    m = buildListResponseHub(msg.sender, msg.vals[LIST_VAL_LEVEL], 1);
                    sendMessage(&m);
                } else {
                    m = buildListResponseHub(msg.sender, msg.vals[LIST_VAL_LEVEL], 0);
                    sendMessage(&m);
                    doListControl(msg.sender, children);
                }
            } else if (msg.type == CLONE_MSG_TYPE) {
                int vals[NVAL] = {id, getpid()};
                message_t m = buildCloneResponse(msg.sender, HUB, vals);
                sendMessage(&m);
            } else if (msg.type == GET_CHILDREN_MSG_TYPE) {
                //  Invio tutti i figli al processo che lo richiede
                message_t m;
                node_t *p = *children;
                while (p != NULL) {
                    m = buildGetChildResponse(msg.sender, p->value);
                    sendMessage(&m);
                    message_t resp;
                    receiveMessage(&resp);
                    p = p->next;
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

message_t buildInfoResponseHub(int sender) {
    // Stato = Override <-> lo stato dei componenti ad esso collegati non sono omogenei (intervento esterno all' HUB)
    node_t *p = *children;
    int count_on = 0, count_off = 0;
    short override = 0;
    while (p != NULL) {
        message_t request = buildInfoRequest(p->value);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error sending info request in buildInfoResponseHub");
        } else if (receiveMessage(&response) == -1) {
            perror("Error receiving info response in buildInfoResponseHub");
        } else {
            if (response.type != INFO_MSG_TYPE) {
                message_t busy = buildBusyResponse(response.sender);
                sendMessage(&busy);
                continue;  // Faccio ripartire il ciclo sullo stesso figlio se leggo un messaggio non pertinente
            }
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
    if (override == 0) {
        if (count_on == 0)
            children_state = 0;
        else if (count_off == 0)
            children_state = 1;
        else
            children_state = (count_off >= count_on) ? 2 : 3;
    } else {
        children_state = (count_off >= count_on) ? 2 : 3;
    }
    message_t ret = buildInfoResponse(sender);
    char *children_str;
    switch (children_state) {
        case 0: children_str = "off"; break;
        case 1: children_str = "on"; break;
        case 2: children_str = "off (override)"; break;
        case 3: children_str = "on (override)"; break;
    }

    sprintf(ret.text, "%s, state: %s", HUB, children_str);
    ret.vals[INFO_VAL_STATE] = children_state;
    return ret;
}

message_t buildListResponseHub(int to_pid, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s %s", HUB, "");
    return ret;
}

void clone(char **argv) {
    int to_clone_pid = atol(argv[2]);
    message_t request = buildGetChildRequest(to_clone_pid);
    message_t response;
    int child_pid;
    sendMessage(&request);
    // Linka tutti i figli dell'hub clonato a sè stesso
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