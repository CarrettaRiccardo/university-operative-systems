#include "../base/device.c"
#include "../include/ipc.h"

short state;
short interruttore;  // valore interruttore che è 1 a 1 con lo stato
unsigned long long on_time;
unsigned long long last_start_time;

void init_data() {
    state = SWITCH_POS_OFF_VALUE;
    interruttore = state;
    on_time = 0;
    last_start_time = time(NULL);
}

void clone_data(char **argv) {
    state = atoi(argv[2]);
    interruttore = state;
    on_time = atoi(argv[3]);
    last_start_time = atoi(argv[4]);
}

int handleSwitchDevice(message_t *msg) {
    int success = -1;
    // Interruttore (light) o generico (da dispositivi di controllo)
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_LIGHT_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_GENERIC_SWITCH_VALUE) {
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {  // Spengo
            // Se è accesa, sommo il tempo di accensione e spengo
            if (interruttore == SWITCH_POS_ON_VALUE) {
                on_time += time(NULL) - last_start_time;
                interruttore = SWITCH_POS_OFF_VALUE;
                state = interruttore;
            }
            success = 1;
        }
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {  // Accendo
            // Se è spenta, accendo e salvo il tempo di accensione
            if (interruttore == SWITCH_POS_OFF_VALUE) {
                last_start_time = time(NULL);
                interruttore = SWITCH_POS_ON_VALUE;
                state = interruttore;
            }
            success = 1;
        }
    }
}

message_t buildInfoResponseDevice(int to_pid) {
    message_t ret = buildInfoResponse(to_pid);
    time_t now = time(NULL);
    unsigned long long work_time = on_time + (now - ((state == SWITCH_POS_OFF_VALUE) ? now : last_start_time));  // Se è spenta ritorno solo "on_time", altrimenti on_time+differenza da quanto accesa
    sprintf(ret.text, "%s, state: %s, registers: time=%llds", BULB, state == 1 ? "on" : "off", work_time);
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, "%s %s", BULB, state == 1 ? "on" : "off");
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[NVAL] = {id, state, on_time, last_start_time};
    return buildCloneResponse(to_pid, BULB, vals);
}