#include "../base/device.c"
#include "../include/ipc.h"

short state;
short interruttore;
int delay;
int temp;
int perc;
int open_time;
int last_open_time;

void init_data() {
    state = SWITCH_POS_OFF_VALUE;         // 0 = chiusa, 1 = aperta
    interruttore = SWITCH_POS_OFF_VALUE;  // 0 = fermo, 1 = apertura/chiusura (torna subito ad off, ma se azionato apre la porta o la chiude)
    delay = 13;                           // tempo di chiusura automatica porta
    temp = 4;                             // temperatura interna
    perc = 22;                            // percentuale riempimento (0-100%)
    open_time = 0;                        // tempo apertura porta
    last_open_time = 0;                   // tempo ultima apertura
}

void clone_data(char **argv) {
    state = atoi(argv[2]);
    interruttore = atoi(argv[3]);
    delay = atoi(argv[4]);
    temp = atoi(argv[5]);
    perc = atoi(argv[6]);
    open_time = atoi(argv[7]);
    last_open_time = atoi(argv[8]);
}

int handleSwitchDevice(message_t *msg) {
    int success = -1;
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_GENERIC_SWITCH_VALUE) {  // interruttore (apri/chiudi) o generico (da hub ai propri figli)
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_VALUE) {                                                         // chiudo
            // controllo se il tempo di chiusura automatica NON è superato
            if (state == SWITCH_POS_ON_VALUE && last_open_time + delay > time(NULL)) {
                // se non lo è (quindi deve ancora chiudersi automaticamente), sommo la differenza di tempo attuale al tempo di apertura...
                open_time += time(NULL) - last_open_time;
                // ...e chiudo la porta
                state = SWITCH_POS_OFF_VALUE;
                interruttore = SWITCH_POS_OFF_VALUE;
            }
            // altrimenti è gia su "off"
            success = 1;
        }
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_VALUE) {  // apro
            // se è chiuso
            if (state == SWITCH_POS_OFF_VALUE) {
                // apro la porta e salvo il tempo di apertura
                last_open_time = time(NULL);
                state = SWITCH_POS_ON_VALUE;
                interruttore = SWITCH_POS_OFF_VALUE;
            }
            // altrimenti è gia su "on"
            success = 1;
        }
    } else {
        if (msg->vals[SWITCH_VAL_LABEL] == LABEL_TERM_VALUE) {  // termostato
            if (msg->vals[SWITCH_VAL_POS] != __INT_MAX__) {     // cambio valore
                temp = msg->vals[SWITCH_VAL_POS];
                success = 1;
                open_time += time(NULL) - last_open_time;
            }
        }
    }
    return success;
}

message_t buildInfoResponseDevice(int to_pid) {
    message_t ret = buildInfoResponse(to_pid);
    time_t now = time(NULL);
    int tot_time = open_time + (now - ((state == 0) ? now : last_open_time));  //se è chiusa ritorno solo "tempo", altrimenti tempo+differenza da quanto accesa
    sprintf(ret.text, "%s, state: %s, registers: time=%ds delay=%ds perc=%d%% temp=%d°C", FRIDGE, state == 1 ? "open" : "closed", tot_time, delay, perc, temp);
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, "%s %s", FRIDGE, state == 1 ? "open" : "closed");
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[NVAL] = {id, state, interruttore, delay, temp, perc, open_time, last_open_time};
    return buildCloneResponse(to_pid, FRIDGE, vals);
}