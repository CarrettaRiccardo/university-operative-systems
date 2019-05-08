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

int id;
list_t children;

int main(int argc, char **argv) {
    id = atoi(argv[1]);
    children = listInit();

    while (1) {
        message_t msg;
        if (receiveMessage(getpid(), &msg) == -1) {
            perror("HUB: Error receive message");
        } else {
            if (strcmp(msg.text, INFO_REQUEST) == 0) {
                // ritorna info
            } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
                // apertura/chiusura
            } else if (strcmp(msg.text, MSG_LINK) == 0) {
                // link
                // (value = id a cui linkare)
            } else if (strcmp(msg.text, MSG_SWITCH) == 0) {
                // switch
                // da gestire
            } else if (strcmp(msg.text, MSG_DELETE_REQUEST) == 0) {
                exit(0);
            } else if (strcmp(msg.text, MSG_LIST) == 0) {
                doList(children, CONTROL_DEVICE, msg.sender);
            }
        }
    }

    return 0;
}
