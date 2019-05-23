#include "../include/ipc.h"

/********************************** Workers **********************************/
int doLink(list_t children, int to_clone_pid, const char *base_dir, short is_terminal) {
    message_t request = buildCloneRequest(to_clone_pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        printf("Error sending clone request to %d from %d: %s\n", to_clone_pid, getpid(), strerror(errno));
        return 0;
    } else if (receiveMessage(&response) == -1) {
        printf("Error receiving clone response in %d from %d: %s\n", getpid(), to_clone_pid, strerror(errno));
        return -1;
    } else {
        int pid = fork();
        char exec_file[50];
        // Figlio
        if (pid == 0) {
            char *args[NVAL + 2];
            args[0] = malloc(sizeof(char) * 50);
            strcat(memcpy(args[0], base_dir, sizeof(char) * 50), response.text);  //  Genero il path dell'eseguibile
            //  Converto i values in string e le mando negli args dell'exec
            int i;
            for (i = 1; i < NVAL + 1; i++) {
                args[i] = malloc(sizeof(char) * 20);
                snprintf(args[i], 20, "%d", response.vals[i - 1]);
            }
            args[NVAL + 1] = NULL;
            if (execvp(args[0], args) == -1) {
                printf("Error: clone failed, execvp %s: %s\n", args[0], strerror(errno));
                return -2;
            }
            for (i = 1; i < NVAL + 1; i++) free(args[i]);
        }
        // Padre
        else {
            if (pid == -1) return -3;
            if (is_terminal) {  // Se è il terminale sto eseguendo un unlink. Metto il dispositivo all'inizio della coda per lasciare il controller come ultimo elemento
                listPushFront(children, &pid, sizeof(int));
            } else {
                listPushBack(children, &pid, sizeof(int));
            }
        }
    }
    return 1;
}

/********************************** Requests **********************************/
message_t buildRequest(int to_pid, short msg_type) {
    message_t ret = {.to = to_pid, .sender = getpid(), .type = msg_type};
    return ret;
}

message_t buildInfoRequest(int to_pid) {
    return buildRequest(to_pid, INFO_MSG_TYPE);
}

message_t buildTranslateRequest(int to_pid, int search_id) {
    message_t ret = buildRequest(to_pid, TRANSLATE_MSG_TYPE);
    ret.vals[TRANSLATE_VAL_ID] = search_id;
    return ret;
}

message_t buildDeleteRequest(int to_pid) {
    return buildRequest(to_pid, DELETE_MSG_TYPE);
}

message_t buildListRequest(int to_pid) {
    return buildRequest(to_pid, LIST_MSG_TYPE);
}

message_t buildCloneRequest(int to_pid) {
    return buildRequest(to_pid, CLONE_MSG_TYPE);
}
message_t buildGetChildRequest(int to_pid) {
    return buildRequest(to_pid, GET_CHILDREN_MSG_TYPE);
}

message_t buildLinkRequest(int to_pid, int to_clone_pid) {
    message_t ret = buildRequest(to_pid, LINK_MSG_TYPE);
    ret.vals[LINK_VAL_PID] = to_clone_pid;
    return ret;
}

message_t buildSwitchRequest(int to_pid, int label_val, int pos_val) {
    message_t ret = buildRequest(to_pid, SWITCH_MSG_TYPE);
    ret.vals[SWITCH_VAL_LABEL] = label_val;
    ret.vals[SWITCH_VAL_POS] = pos_val;
    return ret;
}

/*
    home_pid: pid del componente home
    to_pid: pid del componente al qual il comando è destinato
    label_val: valore della label da modificare
    pos_val: valore da modificare per la posizione dell' interuttore
*/
message_t buildTerminalSwitchRequest(int home_pid, int to_pid, int label_val, int pos_val) {
    message_t ret = buildSwitchRequest(home_pid, label_val, pos_val);
    ret.vals[SWITCH_VAL_DEST] = to_pid;
    return ret;
}

message_t buildSetRequest(int to_pid, int label_val, int val_val) {
    message_t ret = buildRequest(to_pid, SET_MSG_TYPE);
    ret.vals[SET_VAL_LABEL] = label_val;
    ret.vals[SET_VAL_VALUE] = val_val;
    return ret;
}

/********************************** Responses **********************************/
message_t buildResponse(int to_pid, short msg_type) {
    message_t ret = {.to = to_pid, .sender = getpid(), .type = msg_type};
    return ret;
}

message_t buildInfoResponse(int to_pid, int id, int lv, short stop) {
    message_t ret = buildResponse(to_pid, INFO_MSG_TYPE);
    ret.vals[INFO_VAL_ID] = id;
    ret.vals[INFO_VAL_LEVEL] = lv;
    ret.vals[INFO_VAL_STOP] = stop;
    return ret;
}

message_t buildSwitchResponse(int to_pid, int success) {
    message_t ret = buildResponse(to_pid, SWITCH_MSG_TYPE);
    ret.vals[SWITCH_VAL_SUCCESS] = success;
    return ret;
}

message_t buildSetResponse(int to_pid, int success) {
    message_t ret = buildResponse(to_pid, SET_MSG_TYPE);
    ret.vals[SET_VAL_SUCCESS] = success;
    return ret;
}

// pid_found = PID processo trovato, <=0 se non trovato
message_t buildTranslateResponse(int to_pid, int pid_found) {
    message_t ret = buildResponse(to_pid, TRANSLATE_MSG_TYPE);
    ret.vals[TRANSLATE_VAL_ID] = pid_found;
    return ret;
}

/*
    Conferma al mittente l' avvenuta ricezione di un comando di tipo DELETE
*/
message_t buildDeleteResponse(int to_pid, int response) {
    message_t ret = buildResponse(to_pid, DELETE_MSG_TYPE);
    ret.vals[DELETE_VAL_RESPONSE] = response;
    return ret;
}

message_t buildListResponse(int to_pid, int id, int lv, short stop) {
    message_t ret = buildResponse(to_pid, LIST_MSG_TYPE);
    ret.vals[INFO_VAL_ID] = id;
    ret.vals[INFO_VAL_LEVEL] = lv;
    ret.vals[INFO_VAL_STOP] = stop;
    return ret;
}

message_t buildCloneResponse(int to_pid, const char *component_type, int id, const int vals[], short is_control_device) {
    message_t ret = buildResponse(to_pid, CLONE_MSG_TYPE);
    strcpy(ret.text, component_type);
    ret.vals[0] = mqid;
    ret.vals[1] = id;
    if (is_control_device) ret.vals[2] = getpid();
    int s = is_control_device ? 3 : 2;
    int i;
    for (i = s; i < NVAL; i++) ret.vals[i] = vals[i - s];  // Copio i valori nella risposta
    return ret;
}

message_t buildGetChildResponse(int to_pid, int child_pid) {
    message_t ret = buildResponse(to_pid, GET_CHILDREN_MSG_TYPE);
    ret.vals[GET_CHILDREN_VAL_ID] = child_pid;
    return ret;
}

message_t buildLinkResponse(int to_pid, int success) {
    message_t ret = buildResponse(to_pid, LINK_MSG_TYPE);
    ret.vals[LINK_VAL_SUCCESS] = success;
    return ret;
}

message_t buildBusyResponse(const int to) {
    message_t ret = buildResponse(to, BUSY_MSG_TYPE);
    return ret;
}

/********************************** Send/Receive **********************************/
int sendMessage(const message_t *msg) {
    if (msg->to <= 0) return -1;
    return msgsnd(mqid, msg, sizeof(message_t) - sizeof(int), 0);
}

int receiveMessage(message_t *msg) {
    return msgrcv(mqid, msg, sizeof(message_t) - sizeof(int), getpid(), 0);
}

/********************************** Signals **********************************/
int sendGetPidByIdSignal(int to_pid, int id) {
    union sigval sv;
    sv.sival_int = id;
    return sigqueue(to_pid, SIGUSR1, sv);
}

/********************************** Init **********************************/
// Inizializza i componenti per comunicare
void ipcInit(int _mqid) {
    mqid = _mqid;
}

int getMq(int pid) {
    const key_t key = pid;                    //creo id per mailbox
    int ret = msgget(key, 0666 | IPC_CREAT);  //mi "collego" alla mq
    if (ret == -1) {
        perror("Errore connessione mq");
        exit(1);
    }
    return ret;
}

void closeMq(int id) {
    if (msgctl(id, IPC_RMID, NULL) == -1) {
        perror("Errore chiusura mq");
        exit(1);
    }
}

// Traduce un id in un pid
int getPidById(list_t children, int id) {
    node_t *p = children->head;
    while (p != NULL) {
        int to_pid = *(int *)p->value;
        int res = getPidByIdSingle(to_pid, id);
        if (res > 0)
            return res;
        p = p->next;
    }
    return -1;
}

int getPidByIdSingle(int to_pid, int id) {
    message_t request = buildTranslateRequest(to_pid, id);
    message_t response;

    if (sendMessage(&request) == -1) {
        perror("Error get pid by id request");
    } else if (receiveMessage(&response) == -1) {
        perror("Error get pid by id response");
    } else if (response.vals[TRANSLATE_VAL_ID] > 0 && response.type == TRANSLATE_MSG_TYPE) {
        return response.vals[TRANSLATE_VAL_ID];  // Id trovato
    }
    return -1;
}
