#include "../include/ipc.h"
#include <unistd.h>
#include "../include/constants.h"

///////////////////////////////////////////////  WORKERS ///////////////////////////////////////////////


//Implementa il metodo LIST per un dispositivo di Controllo (Hub o Timer)
void doListControl(long to_pid, list_t children) {
    node_t *p = *children;
    while (p != NULL) {
        long son = p->value;
        message_t request = buildListRequest(son);
        if (sendMessage(&request) == -1)
            printf("Errore invio msg LIST al pid %ld: %s\n", son, strerror(errno));

        message_t response;
        int stop = 0;
        do {
            do{ //se ricevo un messaggio diverso da quello che mi aspetto, rispondo BUSY
                if(receiveMessage(&response) == -1)
                    perror("Error list control response");
                if(response.type != LIST_MSG_TYPE){
                    message_t busy = buildBusyResponse(response.sender);
                    sendMessage(&busy);
                }
            }while(response.type!= LIST_MSG_TYPE);

            response.to = to_pid;                // Cambio il destinatario per farlo arrivare a mio padre
            response.vals[LIST_VAL_LEVEL] += 1;  //  Aumento il valore "livello"
            stop = response.vals[LIST_VAL_STOP];
            response.vals[LIST_VAL_STOP] = 0;  //  Tolgo lo stop dalla risposta
            if (stop == 1 && p->next == NULL) {
                //  Ultimo figlio, imposto lo stop
                response.vals[LIST_VAL_STOP] = 1;
            }
            else{
                stop = 0;
            }
            
            sendMessage(&response);
        } while (stop != 1);
        p = p->next;
    }
}

void doLink(list_t children, long to_clone_pid, long sender, const char *base_dir) {
    message_t request = buildCloneRequest(to_clone_pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        printf("Error sending CloneRequest to %ld from %d: %s\n", to_clone_pid, getpid(), strerror(errno));
    } else if (receiveMessage(&response) == -1) {
        printf("Error receiving CloneRequest in %d from %ld: %s\n", getpid(), to_clone_pid, strerror(errno));
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
                snprintf(args[i], 10, "%ld", response.vals[i - 1]);
            }
            args[NVAL + 1] = NULL;
            if (execvp(args[0], args) == -1) {
                perror("Clone error in doLink");
            }
            for (i = 1; i < NVAL + 1; i++) free(args[i]);
        }
        // Padre
        else {
            if (pid != -1) {
                listPush(children, pid);
            }
            //  Attendo una conferma dal figlio clonato e la inoltro al padre.
            message_t ack;
            receiveMessage(&ack);
            ack.to = sender;
            sendMessage(&ack);
        }
    }
}

///////////////////////////////////////////////  REQUEST ///////////////////////////////////////////////
message_t buildRequest(long to_pid, short msg_type) {
    message_t ret = {.to = to_pid, .sender = getpid(), .session = session, .type = msg_type};
    return ret;
}

message_t buildInfoRequest(long to_pid) {
    return buildRequest(to_pid, INFO_MSG_TYPE);
}

message_t buildTranslateRequest(long to_pid, int search_id) {
    message_t ret = buildRequest(to_pid, TRANSLATE_MSG_TYPE);
    ret.vals[TRANSLATE_VAL_ID] = search_id;
    return ret;
}

message_t buildDeleteRequest(long to_pid) {
    return buildRequest(to_pid, DELETE_MSG_TYPE);
}

message_t buildListRequest(long to_pid) {
    return buildRequest(to_pid, LIST_MSG_TYPE);
}

message_t buildCloneRequest(long to_pid) {
    return buildRequest(to_pid, CLONE_MSG_TYPE);
}
message_t buildGetChildRequest(long to_pid) {
    return buildRequest(to_pid, GET_CHILDREN_MSG_TYPE);
}

message_t buildLinkRequest(long to_pid, long to_clone_pid) {
    message_t ret = buildRequest(to_pid, LINK_MSG_TYPE);
    ret.vals[LINK_VAL_PID] = to_clone_pid;
    return ret;
}

message_t buildSwitchRequest(long to_pid, char *label, char *pos) {
    long label_val = __LONG_MAX__;  // 0 = interruttore (generico), 1 = termostato
    long pos_val = __LONG_MAX__;    // 0 = spento, 1 = acceso; x = termostato

    // mappo label (char*) in valori (long) per poterli inserire in un messaggio
    if (strcmp(label, LABEL_LIGHT) == 0) {
        // 0 = interruttore (luce)
        label_val = LABEL_LIGHT_VALUE;
    } else {
        if (strcmp(label, LABEL_OPEN) == 0) {
            // 1 = interruttore (apri/chiudi)
            label_val = LABEL_OPEN_VALUE;
        } else {
            if (strcmp(label, LABEL_TERM) == 0) {
                // 2 = termostato
                label_val = LABEL_TERM_VALUE;
            } else {
                // valore non valido
            }
        }
    }

    if (label_val != __LONG_MAX__) {
        // mappo pos (char*) in valori (long) per poterli inserire in un messaggio
        if (label_val == LABEL_LIGHT_VALUE || label_val == LABEL_OPEN_VALUE) {  // se è un interrutore (luce o apri/chiudi)
            if (strcmp(pos, SWITCH_POS_OFF) == 0) {                             // "off"
                // 0 = spento/chiuso (generico)
                pos_val = SWITCH_POS_OFF_VALUE;
            } else {
                if (strcmp(pos, SWITCH_POS_ON) == 0) {  // "on"
                    // 1 = acceso/aperto (generico)
                    pos_val = SWITCH_POS_ON_VALUE;
                } else {
                    // valore non valido
                }
            }
        } else {
            if (label_val == LABEL_TERM_VALUE && atol(pos) != 0) {  // è un valore valido solo se è un numero e la label è termostato (2)
                // x = valore termostato
                pos_val = atol(pos);
            } else {
                // valore non valido
            }
        }
    }
    message_t ret = buildRequest(to_pid, SWITCH_MSG_TYPE);
    ret.vals[SWITCH_VAL_LABEL] = label_val;
    ret.vals[SWITCH_VAL_POS] = pos_val;
    return ret;
}

///////////////////////////////////////////////  RESPONSE ///////////////////////////////////////////////
message_t buildResponse(long to_pid, short msg_type) {
    message_t ret = {.to = to_pid, .sender = getpid(), .session = session, .type = msg_type};
    return ret;
}
//Metodo generico per info comuni. Ogni componente usa un override del metodo
message_t buildInfoResponse(long to_pid, const char *tipo_componente) {
    message_t ret = buildResponse(to_pid, INFO_MSG_TYPE);
    strcpy(ret.text, tipo_componente);
    return ret;
}

message_t buildSwitchResponse(long to_pid, short success) {
    message_t ret = buildResponse(to_pid, SWITCH_MSG_TYPE);
    ret.vals[SWITCH_VAL_SUCCESS] = success;
    return ret;
}

message_t buildTranslateResponse(long to_pid, long found) {
    message_t ret = buildResponse(to_pid, TRANSLATE_MSG_TYPE);
    ret.vals[TRANSLATE_VAL_FOUND] = found;
    return ret;
}

//Esegue ricorsivamente nei figli di un dispositivo di controllo il TRANSLATE
message_t buildTranslateResponseControl(long sender, int my_id, int search, list_t children){
    if (my_id == search){
        printf("Sono io %ld\n", search);
        return buildTranslateResponse(sender, getppid());
    }
    else{
        long to_pid = getPidById(children,search); //se ho trovato il componente ottengo il suo valore, -1 altrimenti
        printf("Ho trovato %ld\n",to_pid);
        listPrint(children);
        return buildTranslateResponse(sender, to_pid);
    }
}

message_t buildDeleteResponse(long to_pid) {
    return buildResponse(to_pid, DELETE_MSG_TYPE);
}

message_t buildListResponse(long to_pid, const char *component_type, int id, int lv, short state, short stop) {
    message_t ret = buildResponse(to_pid, LIST_MSG_TYPE);
    strcpy(ret.text, component_type);
    ret.vals[LIST_VAL_ID] = id;
    ret.vals[LIST_VAL_LEVEL] = lv;
    ret.vals[LIST_VAL_STATE] = state;
    ret.vals[LIST_VAL_STOP] = stop;
    return ret;
}

message_t buildCloneResponse(long to_pid, const char *component_type, const long vals[]) {
    message_t ret = buildResponse(to_pid, CLONE_MSG_TYPE);
    strcpy(ret.text, component_type);
    int i;
    for (i = 0; i < NVAL; i++) ret.vals[i] = vals[i];  // Copio i valori nella risposta
    return ret;
}

message_t buildGetChildResponse(long to_pid, int child_pid) {
    message_t ret = buildResponse(to_pid, GET_CHILDREN_MSG_TYPE);
    ret.vals[GET_CHILDREN_VAL_ID] = child_pid;
    return ret;
}

message_t buildLinkResponse(long to_pid, short success) {
    message_t ret = buildResponse(to_pid, LINK_MSG_TYPE);
    ret.vals[LINK_VAL_SUCCESS] = success;
    return ret;
}

message_t buildBusyResponse(const long to){
    message_t ret = buildResponse(to, BUSY_MSG_TYPE);
    return ret;
}

message_t buildDieMessage(long to){
    message_t ret = buildResponse(to, DIE_MESG_TYPE);
    return ret;
}


    ////////////////////////////////////////////////////////////////// SEND/RECEIVE //////////////////////////////////////////////////////////////////
int sendMessage(const message_t *msg){
    if (msg->to <= 0) return -1;
    return msgsnd(mqid, msg, sizeof(message_t) - sizeof(long), 0);
}

int receiveMessage(message_t *msg) {
    return msgrcv(mqid, msg, sizeof(message_t) - sizeof(long), getpid(), 0);
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
long getPidById(list_t figli, int id) {
    node_t *p = *figli;
    while (p != NULL) {
        int id_processo = p->value;
        message_t request = buildTranslateRequest(id_processo, id);
        message_t response;
        printf("Chiedo a id = %d\n", id_processo);
        if (sendMessage(&request) == -1) {
            perror("Error get pid by id request");
        } else if (receiveMessage(&response) == -1) {
            perror("Error get pid by id response");
        } else if (response.vals[TRANSLATE_VAL_FOUND] > 0 && response.type == TRANSLATE_MSG_TYPE) {
            return response.vals[TRANSLATE_VAL_FOUND]; // Id trovato
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
            fprintf(log, "TYPE:%s | FROM:%ld | TO:%ld | VALUES:%ld, %ld, %ld, %ld, %ld, %ld\n", msg.text, msg.sender, msg.to, msg.vals[0], msg.vals[1], msg.vals[2], msg.vals[3], msg.vals[4], msg.vals[5]);
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
    printf("to: %ld, sender: %ld, text: %s, v0: %ld, v1: %ld, v2: %ld, v3: %ld, v4: %ld, v5: %ld, session: %ld\n", msg->to, msg->sender, msg->text, msg->vals[0], msg->vals[1], msg->vals[2], msg->vals[3], msg->vals[4], msg->vals[5], msg->session);
}