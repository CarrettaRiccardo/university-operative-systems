#include "../base/control.c"
#include "../include/ipc.h"

void initData() {}

void cloneData(char **vals) {}

//stampa il sotto-albero che parte dal nodo di destinazione del comando utente. Simile a LIST ma stampa il messaggio di INFO
message_t buildInfoResponseControl(int to_pid, char *children_state) {
    /*message_t ret = buildInfoResponse(to_pid);
    sprintf(ret.text, "%s, state: %s", HUB, children_state);
    return ret;*/
    node_t *p = *children;
    while (p != NULL) {
        int son = p->value;
        message_t request = buildInfoRequest(son);
        if (sendMessage(&request) == -1)
            printf("Error sending list control request to pid %d: %s\n", son, strerror(errno));

        message_t response;
        int stop = 0;
        do {
            // TODO: implementare BUSY globalmente
            do {  // Se ricevo un messaggio diverso da quello che mi aspetto, rispondo BUSY
                if (receiveMessage(&response) == -1)
                    perror("Error receiving list control response");
                if (response.type != INFO_MSG_TYPE) {
                    message_t busy = buildBusyResponse(response.sender);
                    sendMessage(&busy);
                }
            } while (response.type != INFO_MSG_TYPE);

            response.to = to_pid;                // Cambio il destinatario per farlo arrivare a mio padre
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

message_t buildListResponseControl(int to_pid, int id, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s", HUB);
    return ret;
}

message_t buildCloneResponseControl(int to_pid, int id) {
    int vals[] = {};
    return buildCloneResponse(to_pid, HUB, id, vals, 1);
}
