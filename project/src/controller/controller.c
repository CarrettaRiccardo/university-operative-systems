#include "../base/control.c"
#include "../include/ipc.h"

void initData() {}

void cloneData(char **vals) {}

int handleSetControl(message_t *msg) {
    // il controler non ha registri da modificare
    return -1;
}

/*
  Genero le info per il comando INFO del componente corrente
*/
message_t buildInfoResponseControl(int to_pid, int id, char *children_state, char *available_labels, char *registers_values, int lv, short stop) {
    message_t ret = buildInfoResponse(to_pid, id, lv, stop);
    sprintf(ret.text, CB_CYAN "%s" C_WHITE ", " CB_WHITE "state: %s" C_WHITE ", " CB_WHITE "labels:" C_WHITE " general, " CB_WHITE "registers:" C_WHITE " num=%d", CONTROLLER, "on" /*state == 1 ? CB_GREEN "on" : CB_RED "off"*/, listCount(children));
    return ret;
}

/*
  Genero le info per il comando LIST del componente corrente
*/
message_t buildListResponseControl(int to_pid, int id, char *children_state, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, CB_WHITE "%s %s" C_WHITE, CONTROLLER, "on" /*state == 1 ? CB_GREEN "on" : CB_RED "off"*/);
    return ret;
}

/*
  Il controller non può essere clonato, quindi non esegue operazioni.
  L'implemetazione non chiamerà mai questo metodo, che però deve eesere efinito per non dare errore
  in compilazione
*/
message_t buildCloneResponseControl(int to_pid, int id) {
    int vals[] = {};
    return buildCloneResponse(to_pid, CONTROLLER, id, vals, 1);
}
