#include "../base/control.c"
#include "../include/ipc.h"

long begin = 0;          //momento temporale di attivazione
long end = 0;            //momento temporale di disattivazione
short waitForBegin = 0;  // 0 = prossimo evento è begin ([end] < NOW < begin < [end]), 1 = prossimo evento è end ([begin] < NOW < end < [begin])

void switchAlarm();

void initData() {
    // associo il metodo switch al segnare alarm per il begin/end automatico (se TIMER)
    signal(SIGALRM, switchAlarm);
}

void cloneData(char **vals) {
    // associo il metodo switch al segnare alarm per il begin/end automatico (se TIMER)
    signal(SIGALRM, switchAlarm);
}


//stampa il sotto-albero che parte dal nodo di destinazione del comando utente. Simile a LIST ma stampa il messaggio di INFO
message_t buildInfoResponseControl(int to_pid, char *children_state) {
    /*message_t ret = buildInfoResponse(to_pid);
    sprintf(ret.text, "%s, state: %s, begin: %ld, end: %ld", TIMER, children_str, begin, end);
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
    sprintf(ret.text, "%s", TIMER);
    return ret;
}

message_t buildCloneResponseControl(int to_pid, int id) {
    int vals[] = {};
    return buildCloneResponse(to_pid, TIMER, id, vals, 1);
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