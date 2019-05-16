#include "../base/control.c"
#include "../include/ipc.h"
#include <time.h>

short state;
struct tm begin;     //momento temporale di attivazione
struct tm end;       //momento temporale di disattivazione
short waitForBegin;  // 0 = prossimo evento è begin ([end] < NOW < begin < [end]), 1 = prossimo evento è end ([begin] < NOW < end < [begin])

void setAlarm();

void initData() {
    // Associo il metodo switch al segnare alarm per il begin/end automatico (se TIMER)
    signal(SIGALRM, setAlarm);
    max_children_count = 1;
    state = SWITCH_POS_OFF_VALUE;
    begin = *localtime(&(time_t){0});// inizializzo a 0 il tempo
    end = *localtime(&(time_t){0});  // inizializzo a 0 il tempo
    waitForBegin = 0;
}

void cloneData(char **vals) {
    signal(SIGALRM, setAlarm);
    max_children_count = 1;
    state = atoi(vals[0]);
    begin = *localtime(&(time_t){atoi(vals[1])});
    end = *localtime(&(time_t){atoi(vals[2])});
    waitForBegin = atoi(vals[3]);
}

message_t buildInfoResponseControl(int to_pid, int id, char *children_state, char *available_labels, int lv, short stop) {
    message_t ret = buildInfoResponse(to_pid, id, lv, stop);
    char beginText[12] = "";
    char endText[12] = "";
    // genero la stringa di testo personalizzata
    strftime(beginText, sizeof(beginText), "%H:%M:%S", &begin);
    strftime(endText, sizeof(endText), "%H:%M:%S", &end);
    sprintf(ret.text, "%s, state: %s, labels: %s, registers: begin=%s, end=%s", TIMER, children_state, available_labels, beginText, endText);
    return ret;
}

message_t buildListResponseControl(int to_pid, int id, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s", TIMER);
    return ret;
}

message_t buildCloneResponseControl(int to_pid, int id) {
    int vals[] = {state, mktime(&begin), mktime(&end), waitForBegin};
    return buildCloneResponse(to_pid, TIMER, id, vals, 1);
}

void setAlarm() {
    if (waitForBegin == 0) {  // Accensione figli
        doSwitchChildren(LABEL_ALL_VALUE, SWITCH_POS_ON_VALUE);
        // Setto il timer di spegnimento (end) se è maggiore dell'accensione (begin)
        if (time(NULL) < mktime(&end)) {
            alarm(mktime(&end) - time(NULL));
            waitForBegin = 1;
        } else {  // Altrimenti termino gli alarm automatici
            alarm(0);
        }
    } else if (waitForBegin == 1) {  // Spegnimentom figli
        doSwitchChildren(LABEL_ALL_VALUE, SWITCH_POS_OFF_VALUE);
        // Setto il timer di accensione (begin) se è maggiore dello spegnimento (end)
        if (time(NULL) < mktime(&begin)) {
            alarm(mktime(&begin) - time(NULL));
            waitForBegin = 0;
        } else {  // Altrimenti termino gli alarm automatici
            alarm(0);
        }
    }
}
