#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../include/ipc.c"  //TODO: Destro linkare libreria ipc.c

int main(int argc, char **argv) {
    const int id = 7; //TODO: Leggere id da parametro
    
    short stato = 0;  //0 = spenta, 1 = accesa
    unsigned long start_time = time(NULL); //tempo accensione lampadina

    if(sessione == 0){ //Sessione diversa da quella corrente. 
        printf("Errore sessione reader = 0"); //TODO: Gestire correttamente la morte  del processo
        exit(1);
    }

    while (1){
        message_t msg = receiveMessage(getpid());

        if(msg.to == -1) //messaggio da ignorare (per sessione diversa/altri casi)
            continue;

        if (strcmp(msg.text, "DIE") == 0){
            message_t m = buildDieResponse(msg.sender);
            sendMessage(m);
            exit(0);
        }
        else if (strcmp(msg.text, "INFO") == 0){
            unsigned long work_time = time(NULL) - start_time;
            message_t m = buildInfoResponse(id,work_time ,stato,msg.sender," bulb");
            sendMessage(m);
        }
        else if (strcmp(msg.text, "TRANSLATE") == 0){
            message_t m = buildTranslateResponse(id,msg.value, msg.sender);
            sendMessage(m);
        }
    }

    return 0;
}

/* Funzioni portate e modificate tutte nella libreria

//Costruisce il messaggio da inviare per rispondeere a una richiesta di Info
message_t buildInfoResponse(const int id, const time_t sessione,const int tempo,const short stato, const int to)
{
    Message ret = {.to = to, .session = sessione, .value = tempo, .state = stato};
    strcpy(ret.text, strcat( strcat( strcat("<#", id) , ">"), " lampadina" ));
    return ret;
}

//Costruisce il messaggio da inviare per rispondeere a una richiesta di Info
message_t buildTranslateResponse(const int id, const time_t sessione, const int searching,const int to)
{
    message_t ret = {.to = to, .session = sessione, .state = 0, .text = "TRANSLATE"}; //messaggio con risposta negativa
    if(id == searching)
        ret.state = 1;  //stava cercando me
    return ret;
}

short int sendMessage(const int mqid, const message_t msg)
{
    if (msg.to == 0)
    {
        printf("Destinatario invalido\n");
        return -1;
    }

    int ret = msgsnd(mqid, &msg, sizeof(Message), 0);

    if (ret == -1)
    {
        printf("Errore invio da controller %s \n", strerror(errno));
        return -1;
    }
    return 1;
}

//TODO: Se leggo DIE... processo morto
//-1 se da ignorare
message_t receiveMessage(const int mqid, const int reader, const time_t current_session)
{
    Message ret;
    int error = msgrcv(mqid, &ret, sizeof(Message), reader, 0);

    if (error == -1){
        printf("Errore ricezione %d", error);
        ret.to = -1;
    }

    if (ret.session != current_session) //messaggio di una sessione precedente rimasto in memoria
        ret.to = -1;
    return ret;
}

key_t getKey()
{
    key_t ret = ftok(KEYFILE, 65);
    if (ret == -1)
    { //TODO :da verirficare
        printf("Errore ottenimento id");
        exit(1);
    }
    return ret;
}

int getMq()
{
    const key_t key = getKey();              //creo id per mailbox
    int ret = msgget(key, 0666 | IPC_CREAT); //mi "collego" alla mq
    if (ret == -1)
    { //TODO :da verirficare
        printf("Errore connessione mq");
        exit(1);
    }
    return ret;
}

void closeMq(const int id)
{
    if (msgctl(id, IPC_RMID, NULL) == -1)
    {
        printf("Errore chiusura mq");
        exit(1);
    }
}


*/