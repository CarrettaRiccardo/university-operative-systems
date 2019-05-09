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

int id;
list_t children;

char *base_dir;

int main(int argc, char **argv) {
    id = atoi(argv[1]);
    children = listInit();

    base_dir = extractBaseDir(argv[0]);
    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) {
            perror("HUB: Error receive message");
        } else {
            if (msg.type == INFO_MSG_TYPE) {
                // ritorna info
            } else if (msg.type == SWITCH_MSG_TYPE) {
                // apertura/chiusura
            } else if (msg.type == LINK_MSG_TYPE) {
                doLink(children, msg.vals[0], msg.sender, base_dir);
            } else if (msg.type == DELETE_MSG_TYPE) {
                exit(0);
            } else if (msg.type == TRANSLATE_MSG_TYPE) {
                message_t m = buildTranslateResponse(msg.sender, msg.vals[TRANSLATE_VAL_ID] == id ? 1 : 0);
                sendMessage(&m);
            } else if (msg.type == LIST_MSG_TYPE) {
                //  Risponde con i propri dati e inoltra la richiesta ai figli
                message_t m;
                if (listEmpty(children)) {
                    m = buildListResponse(msg.sender, HUB, id, msg.vals[0], 0, 1);
                    sendMessage(&m);
                } else {
                    m = buildListResponse(msg.sender, HUB, id, msg.vals[0], 0, 0);
                    sendMessage(&m);
                    doList(msg.sender, children);
                }
            }
        }
    }

    return 0;
}
