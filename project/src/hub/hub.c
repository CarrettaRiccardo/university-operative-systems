#include "../base/control.c"
#include "../include/ipc.h"

void init_data() {}

void clone_data(char **vals) {}

int handleSwitchControl(message_t *msg, list_t children) {
    printf("TODO");
}

message_t buildInfoResponseControl(int to_pid, list_t children) {
    // Stato = Override <-> lo stato dei componenti ad esso collegati non sono omogenei (intervento esterno all' HUB)
    node_t *p = *children;
    int count_on = 0, count_off = 0;
    short override = 0;
    while (p != NULL) {
        message_t request = buildInfoRequest(p->value);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error sending info request in buildInfoResponseHub");
        } else if (receiveMessage(&response) == -1) {
            perror("Error receiving info response in buildInfoResponseHub");
        } else {
            if (response.type != INFO_MSG_TYPE) {
                message_t busy = buildBusyResponse(response.sender);
                sendMessage(&busy);
                continue;  // Faccio ripartire il ciclo sullo stesso figlio se leggo un messaggio non pertinente
            }
            switch (response.vals[INFO_VAL_STATE]) {
                case 0: count_off++; break;
                case 1: count_on++; break;
                case 2:
                    count_off++;
                    override = 1;
                    break;
                case 3:
                    count_on++;
                    override = 1;
                    break;
            }
        }
        p = p->next;
    }
    short children_state;
    if (override == 0) {
        if (count_on == 0)
            children_state = 0;
        else if (count_off == 0)
            children_state = 1;
        else
            children_state = (count_off >= count_on) ? 2 : 3;
    } else {
        children_state = (count_off >= count_on) ? 2 : 3;
    }
    message_t ret = buildInfoResponse(to_pid);
    char *children_str;
    switch (children_state) {
        case 0: children_str = "off"; break;
        case 1: children_str = "on"; break;
        case 2: children_str = "off (override)"; break;
        case 3: children_str = "on (override)"; break;
    }
    sprintf(ret.text, "%s, state: %s", HUB, children_str);
    ret.vals[INFO_VAL_STATE] = children_state;
    return ret;
}

message_t buildListResponseControl(int to_pid, int id, int lv, short stop) {
    message_t ret = buildListResponse(to_pid, id, lv, stop);
    sprintf(ret.text, "%s", HUB);
    return ret;
}

message_t buildCloneResponseControl(int to_pid, int id) {
    int vals[] = {};
    return buildCloneResponse(to_pid, HUB, id, vals, 1);
}
