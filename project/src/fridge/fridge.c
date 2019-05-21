#include "../base/device.c"
#include "../include/ipc.h"

short state;                // 0 = chiusa, 1 = aperta
short interruttore;         // 0 = fermo, 1 = apertura/chiusura (torna subito ad off, ma se azionato apre la porta o la chiude
int delay;                  // Tempo di chiusura automatica porta
int temp;                   // Temperatura interna
int perc;                   // Percentuale riempimento (0-100%)
int open_time;              // Tempo apertura porta
int last_open_time;         // Tempo ultima apertura
unsigned int missing_time;  // Tempo mancante alla chiusura della porta per poter riprendere correttamente se il sistema è fermato

void closeDoor();
void generalStart();
void generalStop();

void initData() {
    // Associo il metodo switch al segnare alarm per la chiusura automatica dopo il delay
    signal(SIGALRM, closeDoor);
    state = SWITCH_POS_OFF_LABEL_VALUE;
    interruttore = SWITCH_POS_OFF_LABEL_VALUE;
    delay = 10;
    temp = 4;
    perc = 0;
    open_time = 0;
    last_open_time = 0;
    missing_time = 0;
}

void cloneData(char **vals) {
    // Associo il metodo switch al segnare alarm per la chiusura automatica dopo il delay
    signal(SIGALRM, closeDoor);
    state = atoi(vals[0]);
    interruttore = atoi(vals[1]);
    delay = atoi(vals[2]);
    temp = atoi(vals[3]);
    perc = atoi(vals[4]);
    open_time = atoi(vals[5]);
    last_open_time = atoi(vals[6]);
    missing_time = atoi(vals[7]);
    // riaccendo il timer di chiusura automatica se era aperto
    if (state == SWITCH_POS_ON_LABEL_VALUE && delay > 0){
        alarm((time(NULL) - last_open_time) < delay ? delay - (time(NULL) - last_open_time) : delay);
    }
}

int handleSwitchDevice(message_t *msg) {
    int success = SWITCH_ERROR_INVALID_VALUE;
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_OPEN_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_LABEL_VALUE) {  // Chiudo
            // Controllo se il tempo di chiusura automatica NON è superato
            if (state == SWITCH_POS_ON_LABEL_VALUE && last_open_time + delay > time(NULL)) {
                // Se non lo è (quindi deve ancora chiudersi automaticamente), sommo la differenza di tempo attuale al tempo di apertura...
                open_time += time(NULL) - last_open_time;
                // ...e chiudo la porta
                state = SWITCH_POS_OFF_LABEL_VALUE;
                interruttore = SWITCH_POS_OFF_LABEL_VALUE;
                // disabilito il timer di chiusura automatica
                alarm(0);
            }
            // Altrimenti è gia su "off"
            success = 1;
        } else if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {  // Apro
            // Se è chiuso
            if (state == SWITCH_POS_OFF_LABEL_VALUE) {
                // Apro la porta e salvo il tempo di apertura
                last_open_time = time(NULL);
                // setto il timer di chiusura automatica
                if (delay > 0)
                    alarm(delay);
                state = SWITCH_POS_ON_LABEL_VALUE;
                interruttore = SWITCH_POS_OFF_LABEL_VALUE;
            }
            // Altrimenti è gia su "on"
            success = 1;
        }
    } else if (msg->vals[SWITCH_VAL_LABEL] == LABEL_THERM_VALUE) {  // Valore Termostato
        temp = msg->vals[SWITCH_VAL_POS];
        success = 1;
    }
    return success;
}

int handleSetDevice(message_t *msg) {
    int success = -1;
    if (msg->vals[SET_VAL_LABEL] == REGISTER_DELAY_VALUE) {  // Tempo chiusura porta (il cambio sarà effettivo dalla prossima apertura se è già apertou)
        delay = msg->vals[SET_VAL_VALUE];
        success = 1;
    } else if (msg->vals[SET_VAL_LABEL] == REGISTER_PERC_VALUE) {  // Percentuale riempimento
        perc = msg->vals[SET_VAL_VALUE];
        success = 1;
    }
    return success;
}

message_t buildInfoResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildInfoResponse(to_pid, id, lv, 1);
    time_t now = time(NULL);
    int tot_time = open_time + (now - ((state == 0) ? now : last_open_time));  //se è chiusa ritorno solo "tempo", altrimenti tempo+differenza da quanto accesa
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s " C_WHITE ", " CB_WHITE "labels: " C_WHITE "%s %s, " CB_WHITE "registers:" C_WHITE " time=%ds delay=%ds perc=%d%% temp=%d°C", FRIDGE, state == 1 ? CB_GREEN "open" : CB_RED "closed", LABEL_OPEN, LABEL_THERM, tot_time, delay, perc, temp);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_OPEN_VALUE | LABEL_THERM_VALUE;
    ret.vals[INFO_VAL_REG_TIME] = tot_time;
    ret.vals[INFO_VAL_REG_DELAY] = delay;
    ret.vals[INFO_VAL_REG_PERC] = perc;
    ret.vals[INFO_VAL_REG_TEMP] = temp;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_WHITE "%s %s" C_WHITE, FRIDGE, state == 1 ? CB_GREEN "open" : CB_RED "closed");
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[] = {state, interruttore, delay, temp, perc, open_time, last_open_time, missing_time};
    return buildCloneResponse(to_pid, FRIDGE, id, vals, 0);
}

void closeDoor() {
    // Controllo se il tempo di chiusura automatica è arrivato
    if (state == SWITCH_POS_ON_LABEL_VALUE && last_open_time + delay >= time(NULL)) {
        // Se lo è (quindi deve chiudersi automaticamente), sommo la differenza di tempo attuale al tempo di apertura...
        open_time += time(NULL) - last_open_time;
        // ...e chiudo la porta
        state = SWITCH_POS_OFF_LABEL_VALUE;
        interruttore = SWITCH_POS_OFF_LABEL_VALUE;
    }
    // Altrimenti è gia su "off"
}

void generalStart(){
    if (missing_time > 0){
        // riaccendo il timer di chiusura automatica se era aperta la porta
        if (state == SWITCH_POS_ON_LABEL_VALUE && delay > 0){
            alarm(missing_time);
        }
    }
}

void generalStop(){
    // spengo il timer di chiusura automatica e salvo quanto tempo mancava
    missing_time = alarm(0);
}