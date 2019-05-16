#include "../base/control.c"
#include "../include/ipc.h"

short state;

void initData() {
    state = SWITCH_POS_OFF_VALUE;
}

void cloneData(char **vals) {
    state = vals[0];
}

message_t buildInfoResponseControl(int to_pid, char *children_state, char *available_labels) {
    message_t ret = buildInfoResponse(to_pid);
    sprintf(ret.text, "%s, state: %s, labels: %s", HUB, children_state, available_labels);
    return ret;
}

message_t buildListResponseControl(int to_pid, int id, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s", HUB);
    return ret;
}

message_t buildCloneResponseControl(int to_pid, int id) {
    int vals[] = {state};
    return buildCloneResponse(to_pid, HUB, id, vals, 1);
}
