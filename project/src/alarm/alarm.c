#include "../base/device.c"
#include "../include/ipc.h"

short state;
short interruttore;  // valore interruttore che è 1 a 1 con lo stato
int delay;           // Tempo in cui si ricalcola una probabilità di accendersi
int perc;            // Probabilità di accensione automatica ad ogni 'delay'

void ringAlarm();

void initData() {
    // Associo il metodo switch al segnale alarm per l'allarme automatico dopo il delay
    signal(SIGALRM, ringAlarm);
    state = SWITCH_POS_OFF_LABEL_VALUE;
    interruttore = state;
    delay = 10;
    perc = 15;     // Probabilità che l'allarme parti
    alarm(delay);  // lancio l'allarme
}

void cloneData(char **vals) {
    // Associo il metodo switch al segnare alarm per l'allarme automatico dopo il delay
    signal(SIGALRM, ringAlarm);
    state = atoi(vals[0]);
    interruttore = state;
    delay = atoi(vals[1]);
    perc = atoi(vals[2]);
    alarm(delay);  // lancio l'allarme
}

int handleSwitchDevice(message_t *msg) {
    int success = SWITCH_ERROR_INVALID_VALUE;
    // Interruttore generico (da dispositivi di controllo)
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_ALARM_ENABLE_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_LABEL_VALUE) {  // Spengo
            // Se è acceso, spengo
            if (interruttore == SWITCH_POS_ON_LABEL_VALUE) {
                interruttore = SWITCH_POS_OFF_LABEL_VALUE;
                state = interruttore;
                alarm(delay);  // lancio l'allarme
            }
            success = 1;
        }
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {  // Accendo
            // Se è spento, accendo
            if (interruttore == SWITCH_POS_OFF_LABEL_VALUE) {
                interruttore = SWITCH_POS_ON_LABEL_VALUE;
                state = interruttore;
            }
            success = 1;
        }
    }
    return success;
}

int handleSetDevice(message_t *msg) {
    int success = -1;
    if (msg->vals[SET_VAL_LABEL] == REGISTER_DELAY_VALUE) {  // Tempo in cui si ricalcola una probabilità di accendersi (il cambio sarà effettivo dal prossimo ciclo)
        delay = msg->vals[SET_VAL_VALUE];
        alarm(delay);
        success = 1;
    } else if (msg->vals[SET_VAL_LABEL] == REGISTER_PERC_VALUE) {  // Probabilità accensione
        perc = msg->vals[SET_VAL_VALUE];
        success = 1;
    }
    return success;
}

message_t buildInfoResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildInfoResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE " %s, " CB_WHITE "registers:" C_WHITE " delay=%ds perc=%d%%", ALARM, state ? CB_GREEN "ringing" : CB_RED "off", LABEL_ALARM_ENABLE, delay, perc);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_ALARM_ENABLE_VALUE;
    ret.vals[INFO_VAL_REG_TIME] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_DELAY] = delay;
    ret.vals[INFO_VAL_REG_PERC] = perc;
    ret.vals[INFO_VAL_REG_TEMP] = INVALID_VALUE;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_WHITE "%s %s" C_WHITE, ALARM, state == 1 ? CB_GREEN "ringing" : CB_RED "off");
    ret.vals[INFO_VAL_STATE] = state;
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[] = {state, delay, perc};
    return buildCloneResponse(to_pid, ALARM, id, vals, 0);
}

void ringAlarm() {
    // Con una probabilità 'perc' si accende
    srand(time(NULL));
    int r = rand() % 100 + 1;  // 1 - 100
    if (r <= perc) {
        // Se è spento accendo l'allarme
        if (state == SWITCH_POS_OFF_LABEL_VALUE) {
            state = SWITCH_POS_ON_LABEL_VALUE;
            interruttore = SWITCH_POS_ON_LABEL_VALUE;
        }
        // Altrimenti è gia su "on"
    } else {
        // Faccio un altro ciclo
        alarm(delay);
    }
}