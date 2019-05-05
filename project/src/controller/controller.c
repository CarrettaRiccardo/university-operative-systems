#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/list.h"

#define MAX_DEVICE_NAME_LENGTH 20

list_t l;
int next_id;
char *base_dir;

/*#define MAXMSG 20
#define KEYFILE "progfile"
 typedef struct msg {
    int to;
    char text[MAXMSG];
    int value;
    short int state;
    int sender;
    time_t session;
} Message;*/

/*  Inizializza le variabili del controller   */
void controller_init(char *file) {
    l = list_init();
    next_id = 1;
    //  Uso il percorso relativo al workspace, preso da argv[0] per trovare gli altri eseguibili per i device.
    //  Rimuovo il nome del file dal percorso
    base_dir = malloc(sizeof(file) + MAX_DEVICE_NAME_LENGTH);
    strcpy(base_dir, file);
    char *last_slash = strrchr(base_dir, '/');
    if (last_slash) *(last_slash + 1) = '\0';

    //  Inizializzazione mq
    // const time_t sessione = time(NULL);  // get current session of execution. To discriminate different session of execution
    //const int mqid = getMq();
}

/*  Dealloca il controller  */
void controller_destroy() {
    list_destroy(l);
    free(base_dir);
}

/**************************************** LIST ********************************************/
void list_devices() {
    printf("TODO: stampa list\n");
    printf("Lista figli processo:\n");
    list_print(l);
}

/**************************************** ADD ********************************************/
/*  (private) Aggiunge un dispositivo al controller in base al tipo specificato   */
int _add_device(char *file) {
    int pid = fork();
    /*  Processo figlio */
    if (pid == 0) {
        char str_id[10];
        strcat(base_dir, file);               //  Genero il path dell'eseguibile
        snprintf(str_id, 10, "%d", next_id);  //  Converto id in stringa
        char *args[] = {base_dir, str_id, NULL};
        return execvp(args[0], args);
    }
    /*  Processo padre */
    else {
        if (pid != -1) {
            next_id++;
            list_push(l, pid);
        }
        return pid;
    }
}

int add_bulb() {
    return _add_device("bulb");
}

int add_fridge() {
    return _add_device("fridge");
}

int add_window() {
    return _add_device("window");
}

int add_hub() {
    return _add_device("hub");
}

int add_timer() {
    return _add_device("timer");
}

/**************************************** DEL ********************************************/
int del_device(char *id) {
    printf("TODO: del device %s\n", id);
}

/**************************************** LINK ********************************************/
int link_devices(char *id1, char *id2) {
    printf("TODO: link device %s to %s\n", id1, id2);
}

/**************************************** SWITCH ********************************************/
int switch_device(char *id, char *label, char *pos) {
    printf("TODO: switch id: %s, label: %s, pos: %s\n", id, label, pos);
}

/**************************************** INFO ********************************************/
void info_device(char *id) {
    printf("TODO: info device %s\n", id);
    // esecuzione
    /* Message m = buildInfoRequest(mqid, sessione, param[0]);
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
                    }*/
}

/*
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
    */
