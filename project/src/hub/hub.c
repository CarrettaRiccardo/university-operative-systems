#include "../base/control.c"
#include "../include/ipc.h"

void initData() {}

void cloneData(char **vals) {}

message_t buildInfoResponseControl(int to_pid, char *children_state) {
    message_t ret = buildInfoResponse(to_pid);
    sprintf(ret.text, "%s, state: %s", HUB, children_state);
    return ret;
}

message_t buildListResponseControl(int to_pid, int id, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s", HUB);
    return ret;
}

message_t buildCloneResponsseControl(int to_pid, int id) {
    int vals[] = {};
    return buildCloneResponse(to_pid, HUB, id, vals, 1);
}
