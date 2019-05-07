#include "../include/ipc.h"

// Inizializza i componenti per comunicare
void ipc_init() {
    sessione = time(NULL);
    mqid = getMq();
}

/*
* worker dei metodi
* Comandi 'do' che implemantano i comandi dell' utente
*/

/*
I dispositivi di interazione inoltrano la richiesta a tutti i filgi prima di tornare value5=1 al padre, il quale procede ad inviare la richiesta ad un altro dei suoi figli
TODO: 
*/
void doList(list_t figli, const char *mode, const long responde_to) {
    if (strcmp(mode, "CONTROLLER") == 0) {
        node_t *p = *figli;
        while (p != NULL) {
            long son = p->value;

            message_t req = buildListRequest(son);
            if (sendMessage(&req) == -1) printf("Errore invio msg LIST al pid %ld: %s\n", son, strerror(errno));
            message_t response;
            do {
                if (receiveMessage(getpid(), &response) != -1) {
                    printListMessage(&response);  //TODO: Controllare sia un messaggio di LIST e non di altro tipo
                }
            } while (response.value5 != 1);

            p = p->next;
        }
    } else if (strcmp(mode, "CONTROL") == 0) {
        //Nei dispositivi di CONTROLLO bisogna iniziare un messaggio al padre con i propri dati con value5=0 per indentare correttamente
        node_t *p = *figli;
        while (p != NULL) {
            long son = p->value;

            message_t req = buildListRequest(son);
            if (sendMessage(&req) == -1)
                printf("Errore invio msg LIST al pid %ld: %s\n", son, strerror(errno));

            message_t response;
            do {
                receiveMessage(getpid(), &response);
                //TODO: Controllare sia un messaggio di LIST e non di altro tipo
                response.to = responde_to;  //cambio il destinatario per farlo arrivare al Controller
                sendMessage(&response);
            } while (response.value5 != 1);
            p = p->next;
        }
        message_t req = buildListResponse(responde_to, "CONTROL", -1, -1, 1, -1);  //Comando di stop dell' HUB o del TIMER
    }
}

void printMsg(const message_t *msg) {
    printf("to: %ld, sender: %ld, text: %s, v1: %ld, v2: %ld, v3: %d, v4: %d, v5: %d, v6: %d, session: %ld\n", msg->to, msg->sender, msg->text, msg->value1, msg->value2, msg->value3, msg->value4, msg->value5, msg->value6, msg->session);
}

//Metodo di comodo per stampare le Info da mostrare nel comando LIST
void printListMessage(const message_t const *msg) {
    if (strcmp(msg->text, "CONTROL") == 0) return;  // Se è un dispositivo di controllo non devo stampare le sue info, indica solo fine scansione di quel sotto_albero
    int i;
    for (i = 0; i < msg->value1; i++) printf("\t");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)

    printf("| <%d> %s ", msg->value3, msg->text);
    if (strcmp(msg->text, "Bulb") == 0) {
        if (msg->value6 == 0)
            printf(" on\n");
        else if (msg->value6 == 1)
            printf(" off\n");
        else if (msg->value6 == 2)
            printf(" on manually\n");
        else if (msg->value6 == 3)
            printf(" off manually\n");
    } else {
        if (msg->value6 == 0)
            printf(" open\n");
        else if (msg->value6 == 1)
            printf(" close\n");
        else if (msg->value6 == 2)
            printf(" open manually\n");
        else if (msg->value6 == 3)
            printf(" off manually\n");
    }
}

/*
* builder richieste
* Metodi di comodo per costrutire le strutture dei messaggi di request
*/

message_t buildInfoRequest(list_t figli, const long to_id) {
    long to_pid = getPidById(figli, to_id);
    if (to_pid == -1)
        printf("Id %ld non trovato\n", to_id);

    message_t ret = {.to = to_pid, .session = sessione, .text = INFO_REQUEST, .sender = getpid()};
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

/*
* builder risposte
* Metodi di comodo per costrutire le strutture dei messaggi di response
*/

//Metodo generico per info comuni. Ogni componente usa un override del metodo
message_t buildInfoResponse(const long id, const short stato, const int to, const char *tipo_componente) {
    message_t ret = {.to = to, .session = sessione, .value6 = stato, .sender = getpid()};
    strcpy(ret.text, tipo_componente);
    return ret;
}

//state = 1  --> il componente cercato sono io
message_t buildTranslateResponse(const long id, const int searching, const int to) {
    message_t ret = {.to = to, .session = sessione, .value6 = 0, .text = MSG_TRANSLATE, .sender = getpid()};  //messaggio con risposta negativa
    if (id == searching)
        ret.value6 = 1;  //stava cercando me, risposta positiva
    return ret;
}

message_t buildDieResponse(const long to) {
    message_t ret = {.to = to, .session = sessione, .text = MSG_DELETE_RESPONSE, .sender = getpid()};
    return ret;
}

message_t buildListResponse(const long to_pid, const char *nome, const short stato, const long livello, const short stop, const short id) {
    message_t ret = {.to = to_pid, .session = sessione, .value6 = stato, .value5 = stop, .value3 = id, .sender = getpid()};
    strcpy(ret.text, nome);
    ret.value1 = livello + 1;
    return ret;
}

/*
*
* Gestori Message Queue
*
*/

short int sendMessage(const message_t *msg) {
    if (msg->to == 0) {
        printf("Errore: destinatario invalido\n");
        return -1;
    }
    int ret = msgsnd(mqid, msg, sizeof(message_t) - sizeof(long), 0);
    return ret;
}

// to = -1 se il messaggio è da ignorare
int receiveMessage(const long reader, message_t *msg) {
    long old = reader;
    int ret = msgrcv(mqid, msg, sizeof(message_t) - sizeof(long), reader, 0);
    /*if (msg->session != sessione) {  // Messaggio di una sessione precedente rimasto in memoria
        return -1;
    }*/
    return ret;
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

/*
*
* tool traduzione da id (interno al sistema) a pid del S.O
* TODO: gestione cache per ottimizzare la traduzione di componenti già risolte (facoltativo)
*
*/

//traduce un id in un pid
long getPidById(list_t figli, const int id) {
    long ret = -1;
    node_t *p = *figli;

    //TODO: Destro controlla iterazione lsta
    while (ret == -1 && p != NULL) {
        int id_processo = p->value;
        message_t msg = {.to = id_processo, .session = sessione, .text = MSG_TRANSLATE};

        if (sendMessage(&msg) == -1) {
            perror("Errore comunicazione, riprovare");
            break;
        }

        message_t response;
        if (receiveMessage(getpid(), &response) == -1)
            continue;
        if (response.value6 == 1 && strcmp(response.text, MSG_TRANSLATE) == 0)  //trovato l'id che stavo cercando
            ret = response.sender;

        p = p->next;
    }
    return ret;
}

// stampa nel file con nome della session il messaggio
int printLog(message_t msg){
    char f_name[30];
    int ret = -1;
    strcpy(f_name,strcat(msg.session,".txt"));
    FILE* log = fopen(f_name, "a");// crea se non esiste
    if (log != NULL){
        fprintf(log, "TYPE:%s | FROM:%ld | TO:%ld | VALUES:%ld, %ld, %ld, %ld, %ld, %ld\n", msg.text, msg.sender, msg.to, msg.value1, msg.value2, msg.value3, msg.value4, msg.value5, msg.value6);
        // chiudo subito per evitare conflitti di apertura
        fclose(log);
        ret = 0;
    }
    else{
        // error opening file
    }
    return ret;
}
