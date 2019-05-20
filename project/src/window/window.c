#include "../base/device.c"
#include "../include/ipc.h"

short state;
short interruttore;
int open_time;
int last_open_time;

void initData() {
    state = SWITCH_POS_OFF_LABEL_VALUE;
    interruttore = state;
    open_time = 0;
    last_open_time = time(NULL);
}

void cloneData(char **vals) {
    state = atoi(vals[0]);
    interruttore = state;
    open_time = atoi(vals[1]);
    last_open_time = atoi(vals[2]);
}

int handleSwitchDevice(message_t *msg) {
    int success = SWITCH_ERROR_INVALID_VALUE;
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {  // interruttore (apri/chiudi) o generico (da hub ai propri figli)
        // Apro/chiudo (invertendo) solo se preme "on" in quanto l'interruttore sarà sempre "off"
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {
            // Se è chiuso, apro e salvo il tempo di apertura
            if (state == SWITCH_POS_OFF_LABEL_VALUE) {
                last_open_time = time(NULL);
                state = SWITCH_POS_ON_LABEL_VALUE;
                interruttore = SWITCH_POS_OFF_LABEL_VALUE;
            } else {
                // Se è aperto, sommo il tempo di apertura e chiudo
                if (state == SWITCH_POS_ON_LABEL_VALUE) {
                    open_time += time(NULL) - last_open_time;
                    state = SWITCH_POS_OFF_LABEL_VALUE;
                    interruttore = SWITCH_POS_OFF_LABEL_VALUE;
                }
            }
            success = 1;
        } else {
            // Se inserisce "off" non deve fare nulla in quanto l'interruttore è sempre a "off"
            if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_LABEL_VALUE) {
                success = 1;
            }
        }
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
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE " %s, " CB_WHITE "registers:" C_WHITE " time=%ds", WINDOW, state == 1 ? CB_GREEN "open" : CB_RED "closed", LABEL_OPEN, tot_time);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_OPEN_VALUE;
    ret.vals[INFO_VAL_REG_TIME] = tot_time;
    ret.vals[INFO_VAL_REG_DELAY] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_PERC] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_TEMP] = INVALID_VALUE;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_WHITE "%s %s" C_WHITE, WINDOW, state == 1 ? CB_GREEN "open" : CB_RED "closed");
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[NVAL] = {state, open_time, last_open_time};
    return buildCloneResponse(to_pid, WINDOW, id, vals, 0);
}