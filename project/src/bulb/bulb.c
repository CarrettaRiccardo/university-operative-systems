#include "../base/device.c"
#include "../include/ipc.h"

short state;
short interruttore;  // Valore interruttore che è 1 a 1 con lo stato
int on_time;
int last_on_time;

void initData() {
    state = SWITCH_POS_OFF_LABEL_VALUE;
    interruttore = state;
    on_time = 0;
    last_on_time = time(NULL);
}

void cloneData(char **vals) {
    state = atoi(vals[0]);
    interruttore = state;
    on_time = atoi(vals[1]);
    last_on_time = atoi(vals[2]);
}

int handleSwitchDevice(message_t *msg) {
    int success = SWITCH_ERROR_INVALID_VALUE;
    // Interruttore (light) o generico (da dispositivi di controllo)
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_LIGHT_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_LABEL_VALUE) {  // Spengo
            // Se è accesa, sommo il tempo di accensione e spengo
            if (interruttore == SWITCH_POS_ON_LABEL_VALUE) {
                on_time += time(NULL) - last_on_time;
                interruttore = SWITCH_POS_OFF_LABEL_VALUE;
                state = interruttore;
            }
            success = 1;
        } else if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {  // Accendo
            // Se è spenta, accendo e salvo il tempo di accensione
            if (interruttore == SWITCH_POS_OFF_LABEL_VALUE) {
                last_on_time = time(NULL);
                interruttore = SWITCH_POS_ON_LABEL_VALUE;
                state = interruttore;
            }
            success = 1;
        }
    }
    return success;
}

int handleSetDevice(message_t *msg) {
    // La bulb non ha registri da modificare
    return -1;
}

message_t buildInfoResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildInfoResponse(to_pid, id, lv, 1);
    time_t now = time(NULL);
    int tot_time = on_time + (now - ((state == SWITCH_POS_OFF_LABEL_VALUE) ? now : last_on_time));  // Se è spenta ritorno solo "on_time", altrimenti on_time+differenza da quanto accesa
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE " %s, " CB_WHITE "registers:" C_WHITE " time=%ds", BULB, state ? CB_GREEN "on" : CB_RED "off", LABEL_LIGHT, tot_time);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_LIGHT_VALUE;
    ret.vals[INFO_VAL_REG_TIME] = tot_time;
    ret.vals[INFO_VAL_REG_DELAY] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_PERC] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_TEMP] = INVALID_VALUE;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_WHITE "%s %s" C_WHITE, BULB, state ? CB_GREEN "on" : CB_RED "off");
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[] = {state, on_time, last_on_time};
    return buildCloneResponse(to_pid, BULB, id, vals, 0);
}