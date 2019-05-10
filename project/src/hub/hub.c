/*
TODO: Remove not-allowed libraries
*/
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

//Override del metodo in IPC.C per il componente Hub
message_t buildInfoResponseHub(const int sender);

int main(int argc, char **argv) {
    base_dir = extractBaseDir(argv[0]);
    id = atoi(argv[1]);
    children = listInit();
    if (argc > 2) {  //  Clone dell'hub
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
                    m = buildListResponse(msg.sender, HUB, id, msg.vals[0], 0, 1);
                    sendMessage(&m);
                } else {
                    m = buildListResponse(msg.sender, HUB, id, msg.vals[0], 0, 0);
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

message_t buildInfoResponseHub(const long sender) {
    return buildInfoResponse(sender, HUB);
}
