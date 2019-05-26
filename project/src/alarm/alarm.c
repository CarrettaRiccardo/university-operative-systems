#include "../base/device.c"
#include "../include/ipc.h"

#define PROBABILITY_SECONDS 10  // tempo di richiamo della funzione che con probabilità 'prob' accende l'allarme

short state;
short interruttore;  // valore interruttore che è 1 a 1 con lo stato
int delay;           // Tempo in cui si ricalcola una probabilità di accendersi
int prob;            // Probabilità di accensione automatica ad ogni 'delay'
int on_time;         // Tempo di accensione totale dell'allarme
int last_on_time;    // Tempo di ultima accensione per calcolare la differenza

void ringAlarm();

void initData() {
    srand(time(NULL));
    // Associo il metodo ringAlarm al segnale alarm (con probabilità dell'attivazione dell'allarme dopo ogni x)
    signal(SIGALRM, ringAlarm);
    state = SWITCH_POS_OFF_LABEL_VALUE;
    interruttore = state;
    delay = 30;  // tempo di spegnimento allarme se attivato
    prob = 5;    // Probabilità che l'allarme suoni, all'inizio = 5%
    on_time = 0;
    last_on_time = 0;
    alarm(PROBABILITY_SECONDS);  // lancio la funzione che potrebbe attivare l'allarme ogni x
}

void cloneData(char **vals) {
    srand(time(NULL));
    // Associo il metodo switch al segnare alarm per l'allarme automatico dopo il delay
    signal(SIGALRM, ringAlarm);
    state = atoi(vals[0]);
    interruttore = state;
    delay = atoi(vals[1]);
    prob = atoi(vals[2]);
    on_time = atoi(vals[3]);
    last_on_time = atoi(vals[4]);
    if (state == SWITCH_POS_ON_LABEL_VALUE && delay > 0) {
        alarm((time(NULL) - last_on_time) < delay ? delay - (time(NULL) - last_on_time) : delay);
    }
}

int handleSwitchDevice(message_t *msg) {
    int success = SWITCH_ERROR_INVALID_VALUE;
    if (msg->vals[SWITCH_VAL_LABEL] == LABEL_ALARM_ENABLE_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_LABEL_VALUE) {  // Spengo
            // Se è acceso, spengo
            if (state == SWITCH_POS_ON_LABEL_VALUE) {
                state = SWITCH_POS_OFF_LABEL_VALUE;
                interruttore = SWITCH_POS_OFF_LABEL_VALUE;
                on_time += time(NULL) - last_on_time;

                alarm(PROBABILITY_SECONDS);  // riparto col ciclo di probabilità
            }
            success = 1;
        }
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {  // Accendo
            // Se è spento, accendo
            if (state == SWITCH_POS_OFF_LABEL_VALUE) {
                state = SWITCH_POS_ON_LABEL_VALUE;
                interruttore = SWITCH_POS_ON_LABEL_VALUE;
                last_on_time = time(NULL);
                // attendo il 'delay' per lo spegnimento automatico
                if (delay > 0)
                    alarm(delay);
            }
            success = 1;
        }
    }
    return success;
}

int handleSetDevice(message_t *msg) {
    int success = -1;
    if (msg->vals[SET_VAL_REGISTER] == REGISTER_DELAY_VALUE) {  // Tempo speginmento automatico
        int old_delay = delay;
        delay = msg->vals[SET_VAL_VALUE];
        if (state == SWITCH_POS_ON_LABEL_VALUE) {
            int remaining_delay = alarm();
            int new_remaining = remaining_delay + (delay - old_delay);
            if (new_remaining <= 0)  // Se ho già superato il nuovo valore del delay, spengo immediatamente l'alarm
                ringAlarm();
            else  // Altrimenti imposto un nuovo alarm con il valore aggiornato
                alarm(new_remaining);
        }
        success = 1;
    } else if (msg->vals[SET_VAL_REGISTER] == REGISTER_PROB_VALUE) {  // Probabilità accensione
        prob = msg->vals[SET_VAL_VALUE];
        success = 1;
    }
    return success;
}

message_t buildInfoResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildInfoResponse(to_pid, id, lv, 1);
    time_t now = time(NULL);
    int tot_time = on_time + (now - ((state == SWITCH_POS_OFF_LABEL_VALUE) ? now : last_on_time));  // Se è spento ritorno solo "on_time", altrimenti on_time+differenza da quanto acceso
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE " %s, " CB_WHITE "registers:" C_WHITE " time=%ds delay=%ds prob=%d%%", ALARM, state ? CB_GREEN "ringing" : CB_RED "off", LABEL_ALARM_ENABLE, tot_time, delay, prob);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_ALARM_ENABLE_VALUE;
    ret.vals[INFO_VAL_REG_TIME] = tot_time;
    ret.vals[INFO_VAL_REG_DELAY] = delay;
    ret.vals[INFO_VAL_REG_PERC] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_TEMP] = INVALID_VALUE;
    ret.vals[INFO_VAL_REG_PROB] = prob;
    return ret;
}

message_t buildListResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildListResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_CYAN "%s %s" C_WHITE, ALARM, state ? CB_GREEN "ringing" : CB_RED "off");
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_ALARM_ENABLE_VALUE;
    return ret;
}

message_t buildCloneResponseDevice(int to_pid, int id) {
    int vals[] = {state, delay, prob, on_time, last_on_time};
    return buildCloneResponse(to_pid, ALARM, id, vals, 0);
}

void ringAlarm() {
    if (state == SWITCH_POS_OFF_LABEL_VALUE) {  // Se è richiamato perchè deve calcolare la probabilità per accendersi...
        // Con una probabilità 'prob' si accende
        int r = rand() % 100 + 1;  // 1 - 100
        if (r <= prob) {           // Accendo l'allarme
            state = SWITCH_POS_ON_LABEL_VALUE;
            interruttore = SWITCH_POS_ON_LABEL_VALUE;
            last_on_time = time(NULL);
            // Imposto il 'delay' per lo spegnimento automatico
            alarm(delay);
        } else {
            // Faccio un altro ciclo ogni 10 secondi
            alarm(PROBABILITY_SECONDS);
        }
    } else if (state == SWITCH_POS_ON_LABEL_VALUE) {  // ...altrimenti è stato richiamato per chiudersi automaticamente
        state = SWITCH_POS_OFF_LABEL_VALUE;
        interruttore = SWITCH_POS_OFF_LABEL_VALUE;
        on_time += time(NULL) - last_on_time;
        alarm(PROBABILITY_SECONDS);  // Riprendo nel richiamare il ciclo ogni secondo
    }
}
