#include "../base/device.c"
#include "../include/ipc.h"

#define PROBABILITY_SECONDS 10  // tempo di richiamo della funzione che con probabilità 'prob' accende l'allarme

short state;
short interruttore;     // valore interruttore che è 1 a 1 con lo stato
int delay;              // Tempo in cui si ricalcola una probabilità di accendersi
int prob;               // Probabilità di accensione automatica ad ogni 'delay'
int missing_time;       // Tempo rimanente di chisura automatica se viene spento l'interruttore generale
int last_general_stop;  // Ultimo tempo di stop, per calcolare il tempo di apertura in caso sia stoppato mentre la porta è aperta

void ringAlarm();
// metodi richiamati quando si riceve un segnale di "switch <id> general <val>"
void generalStart();
void generalStop();

void initData() {
    // Associo il metodo ringAlarm al segnale alarm (con probabilità dell'attivazione dell'allarme dopo ogni x)
    signal(SIGALRM, ringAlarm);
    state = SWITCH_POS_OFF_LABEL_VALUE;
    interruttore = state;
    delay = 30;  // tempo di spegnimento allarme se attivato
    prob = 5;    // Probabilità che l'allarme suoni, all'inizio = 5%
    missing_time = 0;
    last_general_stop = 0;
    alarm(PROBABILITY_SECONDS);  // lancio la funzione che potrebbe attivare l'allarme ogni x
}

void cloneData(char **vals) {
    // Associo il metodo switch al segnare alarm per l'allarme automatico dopo il delay
    signal(SIGALRM, ringAlarm);
    state = atoi(vals[0]);
    interruttore = state;
    delay = atoi(vals[1]);
    prob = atoi(vals[2]);
    missing_time = atoi(vals[3]);
    last_general_stop = atoi(vals[4]);
    // riaccendo il timer di chiusura automatica se era aperto e se l'interruttore generale è on (last_general_stop = 0; ovvero non è disabilitato)
    if (last_general_stop == 0) {
        if (state == SWITCH_POS_ON_LABEL_VALUE && missing_time > 0) {
            alarm(missing_time);
        } else {
            alarm(PROBABILITY_SECONDS);  // lancio la funzione che potrebbe attivare l'allarme ogni x
        }
    }
}

int handleSwitchDevice(message_t *msg) {
    int success = SWITCH_ERROR_INVALID_VALUE;
    if (msg->vals[SWITCH_VAL_LABEL] != LABEL_GENERAL_VALUE) {
        // Interruttore generico (da dispositivi di controllo)
        if (msg->vals[SWITCH_VAL_LABEL] == LABEL_ALARM_ENABLE_VALUE || msg->vals[SWITCH_VAL_LABEL] == LABEL_ALL_VALUE) {
            if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_LABEL_VALUE) {  // Spengo
                // Se è acceso, spengo
                if (interruttore == SWITCH_POS_ON_LABEL_VALUE) {
                    interruttore = SWITCH_POS_OFF_LABEL_VALUE;
                    state = interruttore;
                    missing_time = 0;
                    alarm(PROBABILITY_SECONDS);  // riparto col ciclo di probabilità
                }
                success = 1;
            }
            if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {  // Accendo
                // Se è spento, accendo
                if (interruttore == SWITCH_POS_OFF_LABEL_VALUE) {
                    interruttore = SWITCH_POS_ON_LABEL_VALUE;
                    state = interruttore;
                    missing_time = delay;
                    // attendo il 'delay' per lo spegnimento automatico
                    if (delay > 0)
                        alarm(delay);
                    else
                        alarm(PROBABILITY_SECONDS);
                }
                success = 1;
            }
        }
    } else {
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_OFF_LABEL_VALUE) {  // Stoppo temporaneamente
            generalStop();
            success = 1;
        }
        if (msg->vals[SWITCH_VAL_POS] == SWITCH_POS_ON_LABEL_VALUE) {  // Riattivo
            generalStart();
            success = 1;
        }
    }
    return success;
}

int handleSetDevice(message_t *msg) {
    int success = -1;
    if (msg->vals[SET_VAL_REGISTER] == REGISTER_DELAY_VALUE) {  // Tempo in cui si ricalcola una probabilità di accendersi (il cambio sarà effettivo dal prossimo ciclo)
        delay = msg->vals[SET_VAL_VALUE];
        // se è già dentro il timer di spegniento automatico, lascio che si spenga e questo cambio sarà effettivo al prossimo ciclo di accensione
        success = 1;
    } else if (msg->vals[SET_VAL_REGISTER] == REGISTER_PROB_VALUE) {  // Probabilità accensione
        prob = msg->vals[SET_VAL_VALUE];
        success = 1;
    }
    return success;
}

message_t buildInfoResponseDevice(int to_pid, int id, int lv) {
    message_t ret = buildInfoResponse(to_pid, id, lv, 1);
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE " %s, " CB_WHITE "registers:" C_WHITE " delay=%ds prob=%d%%", ALARM, state ? CB_GREEN "ringing" : CB_RED "off", LABEL_ALARM_ENABLE, delay, prob);
    ret.vals[INFO_VAL_STATE] = state;
    ret.vals[INFO_VAL_LABELS] = LABEL_ALARM_ENABLE_VALUE;
    ret.vals[INFO_VAL_REG_TIME] = INVALID_VALUE;
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
    int vals[] = {state, delay, prob, missing_time, last_general_stop};
    return buildCloneResponse(to_pid, ALARM, id, vals, 0);
}

void ringAlarm() {
    if (missing_time == 0) {  // se è richiamato perchè deve calcolare la probabilità per accendersi...
        // Con una probabilità 'prob' si accende
        srand(time(NULL));
        int r = rand() % 100 + 1;  // 1 - 100
        if (r <= prob) {
            // Se è spento accendo l'allarme
            if (state == SWITCH_POS_OFF_LABEL_VALUE) {
                state = SWITCH_POS_ON_LABEL_VALUE;
                interruttore = SWITCH_POS_ON_LABEL_VALUE;
                missing_time = delay;
                // attendo il 'delay' per lo spegnimento automatico
                if (delay > 0)
                    alarm(delay);
                else
                    alarm(PROBABILITY_SECONDS);
            }
            // Altrimenti è gia su "on"
        } else {
            // Faccio un altro ciclo ogni secondo
            alarm(PROBABILITY_SECONDS);
        }
    } else {  // ...altrimenti è stato richiamato per chiudersi automaticamente
        if (state == SWITCH_POS_ON_LABEL_VALUE) {
            state = SWITCH_POS_OFF_LABEL_VALUE;
            interruttore = SWITCH_POS_OFF_LABEL_VALUE;
            alarm(PROBABILITY_SECONDS);  // riprendo nel richiamare il ciclo ogni secondo
        }
        missing_time = 0;
    }
}

void generalStart() {
    if (last_general_stop > 0) {  // se era spento (quindi era stata salvato il tempo mancante di spegnimento del delay...)
        // riaccendo il timer di chiusura automatica se era aperta la porta
        if (state == SWITCH_POS_ON_LABEL_VALUE && missing_time > 0) {
            alarm(missing_time);
        } else {
            missing_time = 0;
            alarm(PROBABILITY_SECONDS);
        }
    }
    last_general_stop = 0;
}

void generalStop() {
    if (last_general_stop == 0) {  // se era acceso
        // spengo il timer di chiusura automatica e salvo quanto tempo mancava
        missing_time = alarm(0);
        last_general_stop = time(NULL);
    }
}