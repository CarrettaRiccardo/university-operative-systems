#include <stdio.h>
#include <stdlib.h>

#include "../include/ipc.h"
#include "../include/list.h"
#include "../include/utils.h"

/* Metodi da implemantare nei dispositivi di controllo */
void init_data();
void clone_data(char **argv);
int handleSwitchControl(message_t *msg, list_t children);
message_t buildInfoResponseControl(int to_pid, list_t children);
message_t buildListResponseControl(int to_pid, int id, int lv, short stop);
message_t buildCloneResponseControl(int to_pid, int id);

char *base_dir;
int id;
list_t children;

int main(int argc, char **argv) {
    base_dir = extractBaseDir(argv[0]);
    id = atoi(argv[1]);
    children = listInit();

    if (argc <= 2) {
        // Inizializzazione nuovo control device
        init_data();
    } else {
        // Inzializzazione control device clonato
        clone_data(argv);
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
            perror("Error receiving message in normal device");
        } else {
            switch (msg.type) {
                case TRANSLATE_MSG_TYPE: {
                    message_t m;
                    if (id == msg.vals[TRANSLATE_VAL_ID]) {
                        printf("CONTROL: sono io\n");
                        m = buildTranslateResponse(msg.sender, getpid());
                    } else {
                        // Inoltro la richiesta ai figli
                        int to_pid = getPidById(children, msg.vals[TRANSLATE_VAL_ID]);
                        printf("CONTROL: ho trovato %d\n", to_pid);
                        m = buildTranslateResponse(msg.sender, to_pid);
                    }
                    sendMessage(&m);
                } break;

                case SWITCH_MSG_TYPE: {
                    int success = handleSwitchControl(&msg, children);
                    message_t m = buildSwitchResponse(msg.sender, success);
                    sendMessage(&m);
                } break;

                case INFO_MSG_TYPE: {
                    message_t m = buildInfoResponseControl(msg.sender, children);  // Implementazione specifica dispositivo
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
                    printf("TODO");
                    message_t m = buildDeleteResponse(msg.sender);
                    sendMessage(&m);
                } break;

                case DIE_MESG_TYPE: {
                    //  Rimuovo il mittente di questo messaggio dalla lista dei miei figli
                    listRemove(children, msg.sender);
                } break;
            }
        }
    }
    return 0;
}