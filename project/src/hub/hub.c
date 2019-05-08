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
            if (strcmp(msg.text, INFO_REQUEST) == 0) {
                // ritorna info
            } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
                // apertura/chiusura
            } else if (strcmp(msg.text, MSG_LINK) == 0) {
                doLink(children, msg.vals[0], msg.sender, base_dir);
            } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
                // switch
                // da gestire
            } else if (strcmp(msg.text, MSG_DELETE_REQUEST) == 0) {
                exit(0);
            } else if (strcmp(msg.text, MSG_TRANSLATE) == 0) {
                message_t m = buildTranslateResponse(id, msg.vals[0], msg.sender);
                sendMessage(&m);
            } else if (strcmp(msg.text, MSG_LIST) == 0) {
                message_t m = buildListResponse(msg.sender, HUB, 0, msg.vals[0], 0, id);
                sendMessage(&m);
                doList(children, CONTROL_DEVICE, msg.sender);
            }
        }
    }

    return 0;
}
