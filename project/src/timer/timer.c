#include <time.h>
#include "../base/control.c"
#include "../include/ipc.h"

struct tm begin;     //momento temporale di attivazione
struct tm end;       //momento temporale di disattivazione
short waitForBegin;  // 0 = prossimo evento è begin ([end] < NOW < begin < [end]), 1 = prossimo evento è end ([begin] < NOW < end < [begin])

void setAlarm();

void initData() {
    // Associo il metodo switch al segnare alarm per il begin/end automatico (se TIMER)
    signal(SIGALRM, setAlarm);
    max_children_count = 1;
    state = SWITCH_POS_OFF_LABEL_VALUE;
    begin = *localtime(&(time_t){0});  // inizializzo a 0 il tempo
    end = *localtime(&(time_t){0});    // inizializzo a 0 il tempo
    waitForBegin = 0;
}

void cloneData(char **vals) {
    // Associo il metodo switch al segnare alarm per il begin/end automatico (se TIMER)
    signal(SIGALRM, setAlarm);
    max_children_count = 1;
    state = atoi(vals[0]);
    begin = *localtime(&(time_t){atoi(vals[1])});
    end = *localtime(&(time_t){atoi(vals[2])});
    waitForBegin = atoi(vals[3]);
    alarm(atoi(vals[4]));  // fa ripartire il timer da dove si trovava prima
}

int handleSetControl(message_t *msg) {
    // il timer deve poter modificare begin/end e far partire il timer di accensione/spegnimento
    int success = -1;
    if (msg->vals[SET_VAL_LABEL] == REGISTER_BEGIN_VALUE) {
        time_t event = msg->vals[SET_VAL_VALUE];
        begin = *localtime(&event);
        success = 1;
    } else if (msg->vals[SET_VAL_LABEL] == REGISTER_END_VALUE) {
        time_t event = msg->vals[SET_VAL_VALUE];
        end = *localtime(&event);
        success = 1;
    }
    // set del tempo rimasto per accensione/spegnimento automatica
    if (success != -1) {
        int t_begin = mktime(&begin);
        int t_end = mktime(&end);
        // Setto il timer di spegnimento (end) se è il prossimo evento
        if (time(NULL) < t_end && (t_end < t_begin || t_begin < time(NULL))) {
            alarm(t_end - time(NULL));  // attendo la differenza da ORA
            waitForBegin = 1;
            success = SET_TIMER_STARTED_SUCCESS;
        } else {
            // Setto il timer di accensione (begin) se è il prossimo evento
            if (time(NULL) < t_begin && (t_end > t_begin || t_end < time(NULL))) {
                alarm(t_begin - time(NULL));  // attendo la differenza da ORA
                waitForBegin = 0;
                success = SET_TIMER_STARTED_SUCCESS;
            } else {  // Altrimenti termino gli alarm automatici
                alarm(0);
            }
        }
    }
    return success;
}

message_t buildInfoResponseControl(int to_pid, int id, char *children_state, char *available_labels, char *registers_values, int lv, short stop) {
    message_t ret = buildInfoResponse(to_pid, id, lv, stop);
    char begin_str[12] = "";
    char end_str[12] = "";
    // genero la stringa di testo personalizzata
    strftime(begin_str, sizeof(begin_str), "%H:%M:%S", &begin);
    strftime(end_str, sizeof(end_str), "%H:%M:%S", &end);
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE "%s, " CB_WHITE "registers (max values):" C_WHITE " begin=%s end=%s%s", TIMER, children_state, available_labels, begin_str, end_str, registers_values);
    return ret;
}

message_t buildListResponseControl(int to_pid, int id, char *children_state, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, CB_WHITE "%s %s" C_WHITE, TIMER, children_state);
    return ret;
}

message_t buildCloneResponseControl(int to_pid, int id, int state) {
    int vals[] = {state, mktime(&begin), mktime(&end), waitForBegin, alarm()};
    return buildCloneResponse(to_pid, TIMER, id, vals, 1);
}

void setAlarm() {
    if (waitForBegin == 0) {  // Accensione figli
        doSwitchChildren(LABEL_ALL_VALUE, SWITCH_POS_ON_LABEL_VALUE);
        // Setto il timer di spegnimento (end) se è maggiore dell'accensione (begin)
        if (time(NULL) < mktime(&end)) {
            alarm(mktime(&end) - time(NULL));
            waitForBegin = 1;
        } else {  // Altrimenti termino gli alarm automatici
            alarm(0);
        }
    } else if (waitForBegin == 1) {  // Spegnimentom figli
        doSwitchChildren(LABEL_ALL_VALUE, SWITCH_POS_OFF_LABEL_VALUE);
        // Setto il timer di accensione (begin) se è maggiore dello spegnimento (end)
        if (time(NULL) < mktime(&begin)) {
            alarm(mktime(&begin) - time(NULL));
            waitForBegin = 0;
        } else {  // Altrimenti termino gli alarm automatici
            alarm(0);
        }
    }
}

void doSwitchControl(int label, int pos){
    // stoppa o fa ripartire il 
    if (label == LABEL_GENERAL_VALUE) {  // general
        if (pos == SWITCH_POS_OFF_LABEL_VALUE) {  // spengo
            alarm(0);
        } else if (pos == SWITCH_POS_ON_LABEL_VALUE) {  // accendo
            int t_begin = mktime(&begin);
            int t_end = mktime(&end);
            // Setto il timer di spegnimento (end) se è il prossimo evento
            if (time(NULL) < t_end && (t_end < t_begin || t_begin < time(NULL))) {
                alarm(t_end - time(NULL));  // attendo la differenza da ORA
                waitForBegin = 1;
            } else {
                // Setto il timer di accensione (begin) se è il prossimo evento
                if (time(NULL) < t_begin && (t_end > t_begin || t_end < time(NULL))) {
                    alarm(t_begin - time(NULL));  // attendo la differenza da ORA
                    waitForBegin = 0;
                } else {  // Altrimenti termino gli alarm automatici
                    alarm(0);
                }
            }
        }
    }
}