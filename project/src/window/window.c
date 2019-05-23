#include "../base/device.c"
#include "../include/ipc.h"

short state;
short interruttore_open, interruttore_close;
int open_time;
int last_open_time;

void initData() {
    state = SWITCH_POS_OFF_LABEL_VALUE;
    interruttore_open = state;
    interruttore_close = 1 - interruttore_open;
    open_time = 0;
    last_open_time = time(NULL);
}

void cloneData(char **vals) {
    state = atoi(vals[0]);
    interruttore_open = state;
    interruttore_close = 1 - interruttore_open;
    open_time = atoi(vals[1]);
    last_open_time = atoi(vals[2]);
}

int handleSwitchDevice(message_t *msg) {
    int success = SWITCH_ERROR_INVALID_VALUE;
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_WINDOW_OPEN_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {  // interruttore OPEN o ALL (da hub ai propri figli)
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {
            // Se è chiuso, apro e salvo il tempo di apertura
            if (state == SWITCH_POS_OFF_LABEL_VALUE) {
                last_open_time = time(NULL);
                state = SWITCH_POS_ON_LABEL_VALUE;
                interruttore_open = SWITCH_POS_OFF_LABEL_VALUE;
            }
        }
        // Se è aperto, non faccio nulla
        // Se inserisce "off" non deve fare nulla in quanto l'interruttore è sempre a "off"
        success = 1;
    } else if (msg->vals[SWITCH_VAL_LABEL] == LABEL_WINDOW_CLOSE_VALUE) {  // interruttore CLOSE (da hub ai propri figli)
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {
            // Se è aperto, sommo il tempo di apertura e chiudo
            if (state == SWITCH_POS_ON_LABEL_VALUE) {
                open_time += time(NULL) - last_open_time;
                state = SWITCH_POS_OFF_LABEL_VALUE;
                interruttore_close = SWITCH_POS_OFF_LABEL_VALUE;
            }
        }
        // Se è chiuso, non faccio nulla
        // Se inserisce "off" non deve fare nulla in quanto l'interruttore è sempre a "off"
        success = 1;
    }
    return success;
}

int handleSetDevice(message_t *msg) {
    // la window non ha registri da modificare
    return -1;
}

message_t buildInfoResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildInfoResponse(to_pid, id, lv, 1);
    time_t now = time(NULL);
    int tot_time = open_time + (now - ((state == SWITCH_POS_OFF_LABEL_VALUE) ? now : last_open_time));  // Se è spenta ritorno solo "on_time", altrimenti on_time+differenza da quanto accesa
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE " %s, " CB_WHITE "registers:" C_WHITE " time=%ds", WINDOW, state ? CB_GREEN "open" : CB_RED "closed", LABEL_WINDOW_OPEN " " LABEL_WINDOW_CLOSE, tot_time);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_WINDOW_OPEN_VALUE | LABEL_WINDOW_CLOSE_VALUE;
    ret.vals[INFO_VAL_REG_TIME] = tot_time;
    ret.vals[INFO_VAL_REG_DELAY] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_PERC] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_TEMP] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_PROB] = INVALID_VALUE;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_CYAN "%s %s" C_WHITE, WINDOW, state ? CB_GREEN "open" : CB_RED "closed");
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_WINDOW_OPEN_VALUE | LABEL_WINDOW_CLOSE_VALUE;
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[] = {state, open_time, last_open_time};
    return buildCloneResponse(to_pid, WINDOW, id, vals, 0);
}