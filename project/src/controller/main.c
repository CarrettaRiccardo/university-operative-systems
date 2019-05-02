#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <errno.h>  //TODO: remove unsupported libs

#define MAX_CMD 256

#define MAXMSG 20
#define KEYFILE "progfile"

typedef struct msg {
    int to;
    char text[MAXMSG];
    int value;
    short int state;
    int sender;
    time_t session;
} Message;

int main(int argc, char **argv) {
    const time_t sessione = time(NULL);  // get current session of execution. To discriminate different session of execution
    const int mqid = getMq();            // get access to message queue

    //  Per uscire dal while nel caso si scriva "quit"
    short run = 1;
    printf("Type \"help\" for more information.");
    while (run) {
        printf("> ");

        char cmd[100];
        scanf("%s", &cmd);
        if (strcmp(cmd, "add")) {
        }

        // per poter tener traccia di quale eventuale parametro si sta leggendo e non leggerne oltre se ce ne sono
        int parameter = 0;

        // ----- LIST -----
        if (strcmp(ptr, "list\n") == 0) {
            // esecuzione
            printf("-- Exec LIST");
        }
        // ----- ADD -----
        else if (strcmp(ptr, "add") == 0 || strcmp(ptr, "add\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_ADD][MAX_LEN_PARAMETER];
            // parametri successivi
            while (ptr != NULL && parameter < N_PARAMETERS_ADD) {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
                    // alla prima iterazione copio il parametro
                    strcpy(param[parameter++], ptr);
                    // passo al parametro successivo, ma me ne aspetto solo uno per questo comando
                }
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_ADD || strcmp(param[N_PARAMETERS_ADD - 1], "\n") == 0) {
                printf("Parametri per il comando 'add X' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_ADD);
            } else {
                // esecuzione
                printf("-- Exec ADD con parametro %s", param[0]);
            }
        }
        // ----- DEL -----
        else if (strcmp(ptr, "del") == 0 || strcmp(ptr, "del\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_DEL][MAX_LEN_PARAMETER];
            // parametri successivi
            while (ptr != NULL && parameter < N_PARAMETERS_DEL) {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
                    // alla prima iterazione copio il parametro
                    strcpy(param[parameter++], ptr);
                    // passo al parametro successivo, ma me ne aspetto solo uno per questo comando
                }
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_DEL || strcmp(param[N_PARAMETERS_DEL - 1], "\n") == 0) {
                printf("Parametri per il comando 'del X' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_DEL);
            } else {
                // esecuzione
                printf("-- Exec DEL con parametro %s", param[0]);
            }
        }
        // ----- LINK -----
        else if (strcmp(ptr, "link") == 0 || strcmp(ptr, "link\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_LINK][MAX_LEN_PARAMETER];
            // parametri successivi
            while (ptr != NULL && parameter < N_PARAMETERS_LINK) {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
                    // alla prima iterazione copio il parametro
                    strcpy(param[parameter++], ptr);
                    // passo al parametro successivo
                }
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_LINK || strcmp(param[1], "to") != 0 || strcmp(param[N_PARAMETERS_LINK - 1], "\n") == 0) {
                printf("Parametri per il comando 'link X to Y' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_LINK);
            } else {
                // esecuzione
                printf("-- Exec LINK con parametri %s %s %s", param[0], param[1], param[2]);
            }
        }
        // ----- SWITCH -----
        else if (strcmp(ptr, "switch") == 0 || strcmp(ptr, "switch\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_SWITCH][MAX_LEN_PARAMETER];
            // parametri successivi
            while (ptr != NULL && parameter < N_PARAMETERS_SWITCH) {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
                    // alla prima iterazione copio il parametro
                    strcpy(param[parameter++], ptr);
                    // passo al parametro successivo
                }
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_SWITCH || strcmp(param[N_PARAMETERS_SWITCH - 1], "\n") == 0) {
                printf("Parametri per il comando 'switch X Y Z' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_SWITCH);
            } else {
                // esecuzione
                printf("-- Exec SWITCH con parametri %s %s %s", param[0], param[1], param[2]);
            }
        }
        // ----- INFO -----
        else if (strcmp(ptr, "info") == 0 || strcmp(ptr, "info\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_INFO][MAX_LEN_PARAMETER];
            // parametri successivi
            while (ptr != NULL && parameter < N_PARAMETERS_INFO) {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
                    // alla prima iterazione copio il parametro
                    strcpy(param[parameter++], ptr);
                    // passo al parametro successivo, ma me ne aspetto solo uno per questo comando
                }
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_INFO || strcmp(param[N_PARAMETERS_INFO - 1], "\n") == 0) {
                printf("Parametri per il comando 'info X' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_INFO);
            } else {
                // esecuzione
                Message m = buildInfoRequest(mqid, sessione, param[0]);
                if (sendMessage(mqid, m) == -1)
                    printf("Errore comunicazione, riprova");
                Message infos = receiveMessage(mqid, getpid(), sessione);
                if (infos.to != -1) {
                    printf("%s ");
                    if (infos.state == 1)
                        printf("accesa ");
                    else
                        printf("spenta ");
                    printf(". Tempo di funzionamento = %d", infos.value);
                }
            }
        } else if (strcmp(ptr, "exit\n") == 0) {
            printf("-- exec EXIT");
            run = 0;
        } else {
            printf("Comando non riconosciuto");
        }
    }

    return 0;
}

//Costruisce il messaggio da inviare
Message buildInfoRequest(const int mqid, const time_t sessione, const int to_id) {
    int to_pid = getPidById(to_id, sessione, mqid);
    if (to_pid == -1)
        printf("Id non trovato\n");

    Message ret = {.to = to_pid, .session = sessione};
    strcpy(ret.text, "INFO");
    return ret;
}

//traduce un id in un pid con cui è possibile comunicare direttamente
int getPidById(const int id, const time_t sessione, const int mqid) {
    int ret = -1;
    //TODO: Iterare nella lista dei figli
    while (1 && ret == -1) {
        int id_processo = 1;  //TODO: Elemento corrente della lista
        Message msg = {.to = id_processo, .session = sessione};
        strcpy(msg.text, "TRANSLATE");
        if (sendMessage(mqid, msg) == -1) {
            printf("Errore comunicazione, riprovare\n");
            break;
        }

        Message response = receiveMessage(mqid, getpid(), sessione);
        if (response.to == -1)
            continue;
        if (response.state == 1 && strcmp(response.text, "TRANSLATE") == 0)  //trovato l'id che stavo cercando
            ret = response.sender;
    }
    return -1;
}

short int sendMessage(const int mqid, const Message msg) {
    if (msg.to == 0) {
        printf("Destinatario invalido\n");
        return -1;
    }

    int ret = msgsnd(mqid, &msg, sizeof(Message), 0);

    if (ret == -1) {
        printf("Errore invio da controller %s \n", strerror(errno));
        return -1;
    }
    return 1;
}

//TODO: Se leggo DIE... processo morto
//to = -1 se il messaggio è da ignorare
Message receiveMessage(const int mqid, const int reader, const time_t current_session) {
    Message ret;
    int error = msgrcv(mqid, &ret, sizeof(Message), reader, 0);

    if (error == -1) {
        printf("Errore ricezione %d", error);
        ret.to = -1;
    }

    if (ret.session != current_session)  //messaggio di una sessione precedente rimasto in memoria
        ret.to = -1;
    return ret;
}

key_t getKey() {
    key_t ret = ftok(KEYFILE, 65);
    if (ret == -1) {  //TODO :da verirficare
        printf("Errore ottenimento id");
        exit(1);
    }
    return ret;
}

int getMq() {
    const key_t key = getKey();               //creo id per mailbox
    int ret = msgget(key, 0666 | IPC_CREAT);  //mi "collego" alla mq
    if (ret == -1) {                          //TODO :da verirficare
        printf("Errore connessione mq");
        exit(1);
    }
    return ret;
}

void closeMq(const int id) {
    if (msgctl(id, IPC_RMID, NULL) == -1) {
        printf("Errore chiusura mq");
        exit(1);
    }
}
