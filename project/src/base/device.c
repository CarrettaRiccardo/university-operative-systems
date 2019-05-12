#include <stdio.h>
#include <stdlib.h>

#include "../include/ipc.h"

/* Metodi da implemantare nei dispositivi */
void init_data();
void clone_data(char **argv);
int handleSwitchDevice(message_t *msg);
message_t buildInfoResponseDevice(int to_pid);
message_t buildListResponseDevice(int to_pid, int id, int lv);
message_t buildCloneResponseDevice(int to_pid, int id);

int id;
int main(int argc, char **argv) {
    // Inizializzazione
    id = atoi(argv[1]);
    if (argc <= 2) {
        // Inzializzazione nuovo device
        init_data();
    } else {
        // Inzializzazione device clonato
        clone_data(argv);
        message_t confirm_clone = buildLinkResponse(getppid(), 1);
        sendMessage(&confirm_clone);
    }
    // Esecuzione device
    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) {
            perror("Error receiving message in normal device");
        } else {
            switch (msg.type) {
                case TRANSLATE_MSG_TYPE: {
                    message_t m = buildTranslateResponse(msg.sender, msg.vals[TRANSLATE_VAL_ID] == id ? getpid() : -1);
                    sendMessage(&m);
                } break;

                case SWITCH_MSG_TYPE: {
                    int success = handleSwitchDevice(&msg);
                    message_t m = buildSwitchResponse(msg.sender, success);
                    sendMessage(&m);
                } break;

                case INFO_MSG_TYPE: {
                    message_t m = buildInfoResponseDevice(msg.sender);  // Implementazione specifica dispositivo
                    sendMessage(&m);
                } break;

                case LIST_MSG_TYPE: {
                    message_t m = buildListResponseDevice(msg.sender, id, msg.vals[LIST_VAL_LEVEL]);  // Implementazione specifica dispositivo
                    sendMessage(&m);
                } break;

                case LINK_MSG_TYPE: {
                    message_t m = buildLinkResponse(msg.sender, -1);  // I device non di controllo non accettano collegamenti da altri dispositivi
                    sendMessage(&m);
                } break;

                case CLONE_MSG_TYPE: {
                    message_t m = buildCloneResponseDevice(msg.sender, id);  // Implementazione specifica dispositivo
                    sendMessage(&m);
                } break;

                case DELETE_MSG_TYPE: {
                    message_t m = buildDeleteResponse(msg.sender);
                    sendMessage(&m);
                    if (getppid() != msg.sender) {
                        // Sto morendo, invio conferma di ricezione al mittente, e nel caso che il mittente non sia mio padre, invio un messaggio a mio padre di
                        // rimuovermi dalla lista dei suoi figli
                        message_t m = buildDieMessage(getppid());
                        sendMessage(&m);
                    }
                    exit(0);
                } break;
            }
        }
    }
    return 0;
}