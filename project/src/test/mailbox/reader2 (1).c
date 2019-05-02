// Dispositivo intermediario (tipo HUB)
#include <errno.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXMSG 15
#define KEYFILE "progfile"

typedef struct msg {
    long mesg_type;
    char text[MAXMSG];
    short int valore;
    time_t session;
} Message;

short int sendMessage(const int mqid, int to, char msg[MAXMSG], const time_t session) {
    if (to == 0) {
        printf("Destinatario invalido\n");
        return -1;
    }
    Message m = {.mesg_type = to, .session = session};
    strcpy(m.text, msg);

    int ret = msgsnd(mqid, &m, sizeof(Message), 0);

    if (ret == -1) {
        printf("Errore invio da controller %s \n", strerror(errno));
        return -1;
    }
    return 1;
}

Message receiveMessage(const int mqid, const int current_session) {
    Message ret;
    //TODO: int reader = getpid(); //ascolto i messaggi indirizzati a me
    int reader = 2;
    int error = msgrcv(mqid, &ret, sizeof(Message), reader, 0);

    if (error == -1) {
        printf("Errore ricezione %d", error);
        ret.mesg_type = -1;  //messaggio non valido
    }
    if (ret.session != current_session)  //messaggio di una sessione precedente rimasto in memoria
        ret.mesg_type = -1;
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

int main(int argc, char **argv) {
    const int mqid = getMq();
    const int sessione = (int)atoi(argv[1]);

    if (sessione == 0) {
        printf("Errore sessione reader = 0");
        char msg[MAXMSG] = "";

        strcat(msg, "Die ");
        char my_pid[MAXMSG - strlen("Die ")];
        sprintf(my_pid, "%d", getpid());  //converto il pid in stringa

        strncat(msg, my_pid, MAXMSG - strlen("Die "));
        sendMessage(mqid, getppid(), msg, sessione);
        exit(1);
    }

    char *cmd2 = "./reader3";
    char *args[3];
    args[0] = "./reader3";  //per convenzione va ripetuto
    args[1] = (char *)malloc((MAXMSG + 1) * sizeof(char));
    args[2] = NULL;  //obbligatorio NULL finale

    sprintf(args[1], "%d", sessione);

    int f = fork();
    if (f == 0) {
        if (execvp(cmd2, args) == -1) perror("Error exec ");
    } else if (f == -1)
        perror("Error fork ");

    while (1) {
        Message message = receiveMessage(mqid, sessione);
        if (message.mesg_type == -1) {  //messaggio non valido, viene scartato
            printf("scartato");
            continue;
        }
        //TODO: Switch per vari tipi di comandi
        sendMessage(mqid, 3, "Come stai?", sessione);
        Message message_son = receiveMessage(mqid, sessione);
        sendMessage(mqid, getppid(), message_son.text, sessione);
    }

    closeMq(mqid);

    return 0;
}
