#include "../base/control.c"
#include "../include/ipc.h"

short state;
int begin;           //momento temporale di attivazione
int end;             //momento temporale di disattivazione
short waitForBegin;  // 0 = prossimo evento è begin ([end] < NOW < begin < [end]), 1 = prossimo evento è end ([begin] < NOW < end < [begin])

void switchAlarm();

void initData() {
    // Associo il metodo switch al segnare alarm per il begin/end automatico (se TIMER)
    signal(SIGALRM, switchAlarm);
    max_children_count = 1;
    state = SWITCH_POS_OFF_VALUE;
    begin = 0;
    end = 0;
    waitForBegin = 0;
}

void cloneData(char **vals) {
    signal(SIGALRM, switchAlarm);
    max_children_count = 1;
    state = atoi(vals[0]);
    begin = atoi(vals[1]);
    end = atoi(vals[2]);
    waitForBegin = atoi(vals[3]);
}

message_t buildInfoResponseControl(int to_pid, int id, char *children_state, char *available_labels, int lv, short stop) {
    message_t ret = buildInfoResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s, state: %s, labels: %s, registers: begin=%d, end=%d", TIMER, children_state, available_labels, begin, end);
    return ret;
}

message_t buildListResponseControl(int to_pid, int id, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s", TIMER);
    return ret;
}

message_t buildCloneResponseControl(int to_pid, int id) {
    int vals[] = {state, begin, end, waitForBegin};
    return buildCloneResponse(to_pid, TIMER, id, vals, 1);
}

void switchAlarm() {
    if (waitForBegin == 0) {  // Accensione figli
        doSwitchChildren(LABEL_ALL_VALUE, SWITCH_POS_ON_VALUE);
        // Setto il timer di spegnimento (end) se è maggiore dell'accensione (begin)
        if (time(NULL) < end) {
            alarm(end - time(NULL));
            waitForBegin = 1;
        } else {  // Altrimenti termino gli alarm automatici
            alarm(0);
        }
    } else if (waitForBegin == 1) {  // Spegnimentom figli
        doSwitchChildren(LABEL_ALL_VALUE, SWITCH_POS_OFF_VALUE);
        // Setto il timer di accensione (begin) se è maggiore dello spegnimento (end)
        if (time(NULL) < begin) {
            alarm(begin - time(NULL));
            waitForBegin = 0;
        } else {  // Altrimenti termino gli alarm automatici
            alarm(0);
        }
    }
}
