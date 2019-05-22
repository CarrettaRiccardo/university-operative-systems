#include "../base/control.c"
#include "../include/ipc.h"

void initData() {
    state = SWITCH_POS_OFF_LABEL_VALUE;
}

void cloneData(char **vals) {
    state = atoi(vals[0]);
}

int handleSetControl(message_t *msg) {
    // l'hub non ha registri da modificare
    return -1;
}

/* Genero le info per il comando INFO del componente corrente */
message_t buildInfoResponseControl(int to_pid, int id, char *state_str, char *available_labels, char *registers_values, int lv, short stop) {
    message_t ret = buildInfoResponse(to_pid, id, lv, stop);
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state:%s" C_WHITE ", " CB_WHITE "labels:" C_WHITE "%s, " CB_WHITE "registers (max values):" C_WHITE "%s", HUB, state_str, available_labels, registers_values);
    return ret;
}

/* Genero le info per il comando LIST del componente corrente */
message_t buildListResponseControl(int to_pid, int id, char *state_str, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, CB_CYAN "%s" C_WHITE "%s" C_WHITE, HUB, state_str);
    return ret;
}

/**
 * Genero il messaggio di risposta per finalizzare la clonazione di componenti.
 * Copia i valori dei registri per essere ripristinati.
 * HUB non ha registri.
 **/
message_t buildCloneResponseControl(int to_pid, int id, int state) {
    int vals[] = {state};
    return buildCloneResponse(to_pid, HUB, id, vals, 1);
}
