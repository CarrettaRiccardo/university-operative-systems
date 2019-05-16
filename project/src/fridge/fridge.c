#include "../base/device.c"
#include "../include/ipc.h"

short state;         // 0 = chiusa, 1 = aperta
short interruttore;  // 0 = fermo, 1 = apertura/chiusura (torna subito ad off, ma se azionato apre la porta o la chiude
int delay;           // Tempo di chiusura automatica porta
int temp;            // Temperatura interna
int perc;            // Percentuale riempimento (0-100%)
int open_time;       // Tempo apertura porta
int last_open_time;  // Tempo ultima apertura

void initData() {
    state = SWITCH_POS_OFF_VALUE;
    interruttore = SWITCH_POS_OFF_VALUE;
    delay = 10;
    temp = 4;
    perc = 0;
    open_time = 0;
    last_open_time = 0;
}

void cloneData(char **vals) {
    state = atoi(vals[0]);
    interruttore = atoi(vals[1]);
    delay = atoi(vals[2]);
    temp = atoi(vals[3]);
    perc = atoi(vals[4]);
    open_time = atoi(vals[5]);
    last_open_time = atoi(vals[6]);
}

int handleSwitchDevice(message_t *msg) {
    int success = -1;
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {  // Chiudo
            // Controllo se il tempo di chiusura automatica NON è superato
            if (state == SWITCH_POS_ON_VALUE && last_open_time + delay > time(NULL)) {
                // Se non lo è (quindi deve ancora chiudersi automaticamente), sommo la differenza di tempo attuale al tempo di apertura...
                open_time += time(NULL) - last_open_time;
                // ...e chiudo la porta
                state = SWITCH_POS_OFF_VALUE;
                interruttore = SWITCH_POS_OFF_VALUE;
            }
            // Altrimenti è gia su "off"
            success = 1;
        } else if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {  // Apro
            // Se è chiuso
            if (state == SWITCH_POS_OFF_VALUE) {
                // Apro la porta e salvo il tempo di apertura
                last_open_time = time(NULL);
                state = SWITCH_POS_ON_VALUE;
                interruttore = SWITCH_POS_OFF_VALUE;
            }
            // Altrimenti è gia su "on"
            success = 1;
        }
    } else if (msg->vals[SWITCH_VAL_LABEL] == LABEL_TERM_VALUE) {  // Valore Termostato
        temp = msg->vals[SWITCH_VAL_POS];
        success = 1;
    }
    return success;
}

int handleSetDevice(message_t *msg) {
    int success = -1;
    if (msg->vals[SET_VAL_LABEL] == LABEL_DELAY_VALUE) {  // Tempo chiusura porta
        delay = msg->vals[SET_VAL_VALUE];
        success = 1;
    } else if (msg->vals[SET_VAL_VALUE] == LABEL_PERC_VALUE) {  // Percentuale riempimento
        perc = msg->vals[SET_VAL_VALUE];
        success = 1;
    }
    return success;
}

message_t buildInfoResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildInfoResponse(to_pid, id, lv, 1);
    time_t now = time(NULL);
    int tot_time = open_time + (now - ((state == 0) ? now : last_open_time));  //se è chiusa ritorno solo "tempo", altrimenti tempo+differenza da quanto accesa
    sprintf(ret.text, "%s, state: %s, labels: %s %s, registers: time=%ds delay=%ds perc=%d%% temp=%d°C", FRIDGE, state == 1 ? "open" : "closed", LABEL_OPEN, LABEL_TERM, tot_time, delay, perc, temp);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_STOP] = 1;
    ret.vals[INFO_VAL_LABELS] = LABEL_OPEN_VALUE | LABEL_TERM_VALUE;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, "%s %s", FRIDGE, state == 1 ? "open" : "closed");
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[] = {state, interruttore, delay, temp, perc, open_time, last_open_time};
    return buildCloneResponse(to_pid, FRIDGE, id, vals, 0);
}