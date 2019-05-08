#include "../include/ipc.h"
#include "../include/constants.h"

///////////////////////////////////////////////  WORKERS ///////////////////////////////////////////////
void doList(list_t figli, const char *mode, const long responde_to) {
    if (strcmp(mode, CONTROLLER) == 0) {
        node_t *p = *figli;
        while (p != NULL) {
            long son = p->value;

            message_t request = buildListRequest(son);
            if (sendMessage(&request) == -1)
                printf("Errore invio msg LIST al pid %ld: %s\n", son, strerror(errno));
            message_t response;
            do {
                if (receiveMessage(getpid(), &response) != -1) {
                    printListMessage(&response);  //TODO: Controllare sia un messaggio di LIST e non di altro tipo
                }
            } while (response.vals[4] != 1);

            p = p->next;
        }
    } else if (strcmp(mode, CONTROL_DEVICE) == 0) {
        //Nei dispositivi di CONTROLLO bisogna iniziare un messaggio al padre con i propri dati con value5=0 per indentare correttamente
        node_t *p = *figli;
        while (p != NULL) {
            long son = p->value;
            printf("Figlio: %ld\n", son);
            message_t request = buildListRequest(son);
            if (sendMessage(&request) == -1)
                printf("Errore invio msg LIST al pid %ld: %s\n", son, strerror(errno));

            message_t response;
            do {
                receiveMessage(getpid(), &response);
                //TODO: Controllare sia un messaggio di LIST e non di altro tipo
                response.to = responde_to;  //cambio il destinatario per farlo arrivare al Controller
                sendMessage(&response);
            } while (response.vals[4] != 1);
            p = p->next;
        }
        message_t endResponse = buildListResponse(responde_to, CONTROL_DEVICE, -1, -1, 1, -1);  //Comando di stop dell' HUB o del TIMER
        sendMessage(&endResponse);
    }
}

void printMsg(const message_t *msg) {
    printf("to: %ld, sender: %ld, text: %s, v1: %ld, v2: %ld, v3: %ld, v4: %ld, v5: %ld, v6: %ld, session: %ld\n", msg->to, msg->sender, msg->text, msg->vals[0], msg->vals[1], msg->vals[2], msg->vals[3], msg->vals[4], msg->vals[5], msg->session);
}

//Metodo di comodo per stampare le Info da mostrare nel comando LIST
void printListMessage(const message_t const *msg) {
    if (strcmp(msg->text, CONTROL_DEVICE) == 0) return;  // Se è un dispositivo di controllo non devo stampare le sue info, indica solo fine scansione di quel sotto_albero
    int i;
    for (i = 0; i < msg->vals[0]; i++) printf("   ");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)

    printf("| <%ld> %s ", msg->vals[2], msg->text);
    if (strcmp(msg->text, BULB) == 0) {
        switch (msg->vals[5]) {
            case 0: printf(" off\n"); break;
            case 1: printf(" on\n"); break;
            case 2: printf(" off (override)\n"); break;
            case 3: printf(" on (override)\n"); break;
        }
    } else {
        switch (msg->vals[5]) {
            case 0: printf(" open\n"); break;
            case 1: printf(" close\n"); break;
            case 2: printf(" open (override)\n"); break;
            case 3: printf(" close (override)\n"); break;
        }
    }
}

void doLink(list_t figli, long to_clone_pid) {
    message_t request = buildCloneRequest(to_clone_pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        printf("Error sending CloneRequest to %ld from %d: %s\n", to_clone_pid, getpid(), strerror(errno));
    } else if (receiveMessage(getpid(), &response) == -1) {
        printf("Error receiving CloneRequest in %d from %ld: %s\n", getpid(), to_clone_pid, strerror(errno));
    } else {
        int pid = fork();
        // Figlio
        if (pid == 0) {
            char args[NVAL + 1][30];
            int i;
            //  Converto i values in string e le mando negli args dell'exec
            for (i = 1; i < NVAL + 1; i++) {
                snprintf(args[i], 10, "%ld", response.vals[i]);
            }
            char path[40] = "";
            strcpy(args[1], strcat(strcat(path, "./"), response.text));  //  Genero il path dell'eseguibile
            if (execvp(args[0], args) == -1) {
                perror("Clone error");
            }
        }
        // Padre
        else {
            if (pid != -1) {
                listPush(figli, pid);
            }
        }
    }
}

///////////////////////////////////////////////  REQUEST ///////////////////////////////////////////////
message_t buildInfoRequest(list_t figli, const long to_id) {
    long to_pid = getPidById(figli, to_id);
    if (to_pid == -1)
        printf("Id %ld non trovato\n", to_id);

    message_t ret = {.to = to_pid, .session = sessione, .text = INFO_REQUEST, .sender = getpid()};
    return ret;
}

message_t buildTranslateRequest(const long to_pid, const int search) {
    message_t ret = {.to = to_pid, .session = sessione, .text = MSG_TRANSLATE, .sender = getpid(), .vals = {search}};
    if (to_pid == -1)
        printf("Pid %ld non trovato\n", to_pid);
    return ret;
}

message_t buildDieRequest(list_t figli, const long to_id) {
    long to_pid = getPidById(figli, to_id);
    if (to_pid == -1)
        printf("Id %ld non trovato\n", to_id);

    message_t ret = {.to = to_pid, .session = sessione, .text = MSG_DELETE_REQUEST, .sender = getpid()};
    return ret;
}

//Il PID è già noto (preso dalla lista da lista figli), non occorre la traduzione
message_t buildListRequest(const long to_pid) {
    message_t ret = {.to = to_pid, .session = sessione, .text = MSG_LIST, .sender = getpid()};
    return ret;
}

message_t buildCloneRequest(const long to_pid) {
    message_t ret = {.to = to_pid, .session = sessione, .text = MSG_CLONE, .sender = getpid()};
    return ret;
}

message_t buildSwitchRequest(list_t figli, const long to_id, char *label, char *pos) {
    long to_pid = getPidById(figli, to_id);
    long label_val = -1;  // 0 = interruttore (generico), 1 = termostato
    long pos_val = -1;    // 0 = spento, 1 = acceso; x = termostato

    if (to_pid == -1)
        printf("Id %ld non trovato\n", to_id);
    else {
        // mappo label (char*) in valori (long) per poterli inserire in un messaggio
        if (strcmp(label, LABEL_LIGHT) == 0 || (strcmp(label, LABEL_OPEN) == 0)) {
            // 0 = interruttore (generico)
            label_val = 0;
        } else {
            if (strcmp(label, LABEL_LIGHT) == 0) {
                // 1 = termostato
                label_val = 1;
            }
            // altrimenti è un valore non valido
        }

        if (label_val != -1) {
            // mappo pos (char*) in valori (long) per poterli inserire in un messaggio
            if (strcmp(pos, SWITCH_POS_OFF) == 0 && label_val == 0) {
                // 0 = spento/chiuso (generico)
                pos_val = 0;
            } else {
                if (strcmp(pos, SWITCH_POS_ON) == 0 && label_val == 0) {
                    // 1 = acceso/aperto (generico)
                    pos_val = 1;
                }
                // altrimenti è un valore valido solo se è un numero e la label è termostato
                else {
                    if (label_val == 1 && atol(pos) != -1) {
                        pos_val = atol(pos);
                    }
                }
            }
        }
    }
    message_t ret = {.to = to_pid, .session = sessione, .text = MSG_SWITCH, .sender = getpid()};
    ret.vals[0] = label_val;
    ret.vals[1] = pos_val;
    return ret;
}

///////////////////////////////////////////////  RESPONSE ///////////////////////////////////////////////
//Metodo generico per info comuni. Ogni componente usa un override del metodo
message_t buildInfoResponse(const long id, const short stato, const int to, const char *tipo_componente) {
    message_t ret = {.to = to, .session = sessione, .sender = getpid()};
    strcpy(ret.text, tipo_componente);
    ret.vals[5] = stato;
    return ret;
}

message_t buildSwitchResponse(const int success, const int to) {
    message_t ret = {.to = to, .session = sessione, .sender = getpid()};
    ret.vals[5] = success;
    return ret;
}

//state = 1  --> il componente cercato sono io
message_t buildTranslateResponse(const long id, const int searching, const int to) {
    message_t ret = {.to = to, .session = sessione, .text = MSG_TRANSLATE, .sender = getpid()};  //messaggio con risposta negativa
    if (id == searching)
        ret.vals[5] = 1;  //stava cercando me, risposta positiva
    else
        ret.vals[5] = 0;
    return ret;
}

message_t buildDieResponse(const long to) {
    message_t ret = {.to = to, .session = sessione, .text = MSG_DELETE_RESPONSE, .sender = getpid()};
    return ret;
}

message_t buildListResponse(const long to_pid, const char *nome, const short stato, const long livello, const short stop, const short id) {
    message_t ret = {.to = to_pid, .session = sessione, .sender = getpid()};
    strcpy(ret.text, nome);
    ret.vals[0] = livello + 1;
    ret.vals[2] = id;
    ret.vals[4] = stop;
    ret.vals[5] = stato;
    return ret;
}

message_t buildCloneResponse(const long to_pid, const char *type, long vals[]) {
    message_t ret = {.to = to_pid, .session = sessione, .sender = getpid()};
    strcpy(ret.text, type);
    int i;
    for (i = 0; i < NVAL; i++) {
        ret.vals[i] = vals[i];
    }
    return ret;
}

////////////////////////////////////////////////////////////////// SEND/RECEIVE //////////////////////////////////////////////////////////////////
short int sendMessage(const message_t *msg) {
    if (msg->to <= 0) {
        return -1;
    }
    int ret = msgsnd(mqid, msg, sizeof(message_t) - sizeof(long), 0);
    return ret;
}

// to = -1 se il messaggio è da ignorare
int receiveMessage(const long reader, message_t *msg) {
    int ret = msgrcv(mqid, msg, sizeof(message_t) - sizeof(long), reader, 0);
    /*if (msg->session != sessione) {  // Messaggio di una sessione precedente rimasto in memoria
        return -1;
    }*/
    return ret;
}

///////////////////////////////////////////////  INIT ///////////////////////////////////////////////
// Inizializza i componenti per comunicare
void ipcInit() {
    sessione = time(NULL);
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
    const key_t key = getKey();  //creo id per mailbox
    printf("Key mq: %d\n", key);
    int ret = msgget(key, 0666 | IPC_CREAT);  //mi "collego" alla mq
    if (ret == -1) {
        perror("Errore connessione mq");
        exit(1);
    }
    return ret;
}

void closeMq(const int id) {
    if (msgctl(id, IPC_RMID, NULL) == -1) {
        perror("Errore chiusura mq");
        exit(1);
    }
}

// Traduce un id in un pid
long getPidById(list_t figli, const int id) {
    node_t *p = *figli;

    while (p != NULL) {
        int id_processo = p->value;
        message_t request = buildTranslateRequest(id_processo, id);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error get pid by id request");
        } else if (receiveMessage(getpid(), &response) == -1) {
            perror("Error get pid by id response");
        } else if (response.vals[5] == 1 && strcmp(response.text, MSG_TRANSLATE) == 0) {
            //  Id trovato
            return response.sender;
        }
        p = p->next;
    }
    return -1;
}

// stampa nel file con nome della session il messaggio
int printLog(message_t msg) {
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
