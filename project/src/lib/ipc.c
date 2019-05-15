#include "../include/ipc.h"
#include <unistd.h>
#include "../include/constants.h"

///////////////////////////////////////////////  WORKERS ///////////////////////////////////////////////
void doLink(list_t children, int to_clone_pid, const char *base_dir) {
    message_t request = buildCloneRequest(to_clone_pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        printf("Error sending clone request to %d from %d: %s\n", to_clone_pid, getpid(), strerror(errno));
    } else if (receiveMessage(&response) == -1) {
        printf("Error receiving clone response in %d from %d: %s\n", getpid(), to_clone_pid, strerror(errno));
    } else {
        int pid = fork();
        char exec_file[50];
        // Figlio
        if (pid == 0) {
            char *args[NVAL + 2];
            args[0] = malloc(sizeof(char) * 50);
            strcat(strcat(args[0], base_dir), response.text);  //  Genero il path dell'eseguibile
            //  Converto i values in string e le mando negli args dell'exec
            int i;
            for (i = 1; i < NVAL + 1; i++) {
                args[i] = malloc(sizeof(char) * 20);
                snprintf(args[i], 20, "%d", response.vals[i - 1]);
            }
            args[NVAL + 1] = NULL;
            if (execvp(args[0], args) == -1) {
                perror("Clone error in doLink");
            }
            for (i = 1; i < NVAL + 1; i++) free(args[i]);
        }
        // Padre
        else {
            if (pid != -1) listPush(children, pid);
        }
    }
}

///////////////////////////////////////////////  REQUEST ///////////////////////////////////////////////
message_t buildRequest(int to_pid, short msg_type) {
    message_t ret = {.to = to_pid, .sender = getpid(), .session = session, .type = msg_type};
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

///////////////////////////////////////////////  RESPONSE ///////////////////////////////////////////////
message_t buildResponse(int to_pid, short msg_type) {
    message_t ret = {.to = to_pid, .sender = getpid(), .session = session, .type = msg_type};
    return ret;
}
//Metodo generico per info comuni. Ogni componente usa un override del metodo
message_t buildInfoResponse(int to_pid) {
    message_t ret = buildResponse(to_pid, INFO_MSG_TYPE);
    return ret;
}

message_t buildSwitchResponse(int to_pid, short success) {
    message_t ret = buildResponse(to_pid, SWITCH_MSG_TYPE);
    ret.vals[SWITCH_VAL_SUCCESS] = success;
    return ret;
}

// pid_found = PID processo trovato, <=0 se non trovato
message_t buildTranslateResponse(int to_pid, int pid_found) {
    message_t ret = buildResponse(to_pid, TRANSLATE_MSG_TYPE);
    ret.vals[TRANSLATE_VAL_ID] = pid_found;
    return ret;
}

message_t buildTranslateResponseControl(int sender, int my_id, int search, list_t children) {
    if (my_id == search) {
        printf("Sono io\n");
        return buildTranslateResponse(sender, getpid());
    } else {
        int to_pid = getPidById(children, search);
        printf("Ho trovato %d\n", to_pid);
        return buildTranslateResponse(sender, to_pid);
    }
}

message_t buildDeleteResponse(int to_pid) {
    return buildResponse(to_pid, DELETE_MSG_TYPE);
}

message_t buildListResponse(int to_pid, int id, int lv, short stop) {
    message_t ret = buildResponse(to_pid, LIST_MSG_TYPE);
    ret.vals[LIST_VAL_ID] = id;
    ret.vals[LIST_VAL_LEVEL] = lv;
    ret.vals[LIST_VAL_STOP] = stop;
    return ret;
}

message_t buildCloneResponse(int to_pid, const char *component_type, int id, const int vals[], short is_control_device) {
    message_t ret = buildResponse(to_pid, CLONE_MSG_TYPE);
    strcpy(ret.text, component_type);
    ret.vals[0] = id;
    if (is_control_device) ret.vals[1] = getpid();
    int s = is_control_device ? 2 : 1;
    int i;
    for (i = s; i < NVAL; i++) ret.vals[i] = vals[i - s];  // Copio i valori nella risposta
    return ret;
}

message_t buildGetChildResponse(int to_pid, int child_pid) {
    message_t ret = buildResponse(to_pid, GET_CHILDREN_MSG_TYPE);
    ret.vals[GET_CHILDREN_VAL_ID] = child_pid;
    return ret;
}

message_t buildLinkResponse(int to_pid, short success) {
    message_t ret = buildResponse(to_pid, LINK_MSG_TYPE);
    ret.vals[LINK_VAL_SUCCESS] = success;
    return ret;
}

message_t buildBusyResponse(const int to) {
    message_t ret = buildResponse(to, BUSY_MSG_TYPE);
    return ret;
}

////////////////////////////////////////////////////////////////// SEND/RECEIVE //////////////////////////////////////////////////////////////////
int sendMessage(const message_t *msg) {
    if (msg->to <= 0) return -1;
    return msgsnd(mqid, msg, sizeof(message_t) - sizeof(int), 0);
}

int receiveMessage(message_t *msg) {
    return msgrcv(mqid, msg, sizeof(message_t) - sizeof(int), getpid(), 0);
}

///////////////////////////////////////////////  INIT ///////////////////////////////////////////////
// Inizializza i componenti per comunicare
void ipcInit() {
    session = time(NULL);
    mqid = getMq();
}

key_t getKey() {
    key_t ret = ftok(".", 65);
    if (ret == -1) {
        perror("Errore ottenimento key");
        exit(1);
    }
    return ret;
}

int getMq() {
    const key_t key = getKey();               //creo id per mailbox
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
    node_t *p = *children;
    while (p != NULL) {
        int id_processo = p->value;
        message_t request = buildTranslateRequest(id_processo, id);
        message_t response;

        if (sendMessage(&request) == -1) {
            perror("Error get pid by id request");
        } else if (receiveMessage(&response) == -1) {
            perror("Error get pid by id response");
        } else if (response.vals[TRANSLATE_VAL_ID] > 0 && response.type == TRANSLATE_MSG_TYPE) {
            return response.vals[TRANSLATE_VAL_ID];  // Id trovato
        }
        p = p->next;
    }
    return -1;
}
// stampa nel file con nome della session il messaggio
int printLog(const message_t *msg) {
    char f_name[30];
    int ret = -1;
    /*// copio in f_name la msg.session come stringa
    if (snprintf(f_name, sizeof(msg.session), "../log/%s", msg.session) != -1) {
        strcat(f_name, ".txt");
        FILE *log = fopen(f_name, "a");  // crea se non esiste
        if (log != NULL) {
            fprintf(log, "TYPE:%s | FROM:%d | TO:%d | VALUES:%d, %d, %d, %d, %d, %d\n", msg.text, msg.sender, msg.to, msg.vals[0], msg.vals[1], msg.vals[2], msg.vals[3], msg.vals[4], msg.vals[5]);
            // chiudo subito per evitare conflitti di apertura
            fclose(log);
            ret = 0;
        } else {
            // error opening file
            printf("Errore nell'apertura del log");
        }
    }*/
    return ret;
}

void printMsg(const message_t *msg) {
    printf("to: %ld, sender: %d, text: %s, v0: %d, v1: %d, v2: %d, v3: %d, v4: %d, v5: %d, session: %ld\n", msg->to, msg->sender, msg->text, msg->vals[0], msg->vals[1], msg->vals[2], msg->vals[3], msg->vals[4], msg->vals[5], msg->session);
}