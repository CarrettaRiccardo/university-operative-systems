#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/ipc.h"


time_t sessione;
int mqid;



//inizializza i componenti per comunicare
void ipc_init(){
    sessione = time(NULL);
    mqid = getMq();
}



/*
* worker dei metodi
* Comandi 'do' che implemantano i comandi dell' utente
*/

void doList(const char* mode){

}







/*
* builder richieste
* Metodi di comodo per costrutire le strutture dei messaggi di request
*/

message_t buildInfoRequest(const long to_id)
{
    long to_pid = getPidById(to_id);
    if(to_pid == -1)
        printf("Id non trovato\n");

    message_t ret = {.to = to_pid, .session = sessione, .text = "INFO"};
    return ret;
}


message_t buildDieRequest(const long to_id)
{
    long to_pid = getPidById(to_id);
    if(to_pid == -1)
        printf("Id non trovato\n");

    message_t ret = {.to = to_pid, .session = sessione, .text = "DIE"};
    return ret;
}


/*
* builder risposte
* Metodi di comodo per costrutire le strutture dei messaggi di response
*/

message_t buildInfoResponse(const long id, const int valore,const short stato, const int to, const char* tipo_componente)
{
    message_t ret = {.to = to, .session = sessione, .value = valore, .state = stato, .sender = getpid()};
    strcpy(ret.text, tipo_componente);
    return ret;
}

//state = 1  --> il componente cercato sono io
message_t buildTranslateResponse(const long id, const int searching, const int to)
{
    message_t ret = {.to = to, .session = sessione, .state = 0, .text = "TRANSLATE", .sender = getpid()}; //messaggio con risposta negativa
    if(id == searching)
        ret.state = 1;  //stava cercando me, risposta positiva
    return ret;
}

message_t buildDieResponse(const long to)
{
    message_t ret = {.to = to, .session = sessione, .text = "DIED", .sender = getpid()};
    return ret;
}




/*
*
* Gestori Message Queue
*
*/


short int sendMessage(const message_t msg)
{
    if (msg.to == 0)
    {
        printf("Destinatario invalido\n");
        return -1;
    }

    int ret = msgsnd(mqid, &msg, sizeof(message_t), 0);

    if (ret == -1)
    {
        printf("Errore invio da controller %s \n", strerror(errno));
        return -1;
    }
    return 1;
}


//to = -1 se il messaggio è da ignorare
message_t receiveMessage(const long reader)
{
    message_t ret;
    int error = msgrcv(mqid, &ret, sizeof(message_t), reader, 0);

    if (error == -1){
        printf("Errore ricezione %d", error);
        ret.to = -1;
    }

    if (ret.session != sessione) //messaggio di una sessione precedente rimasto in memoria
        ret.to = -1;
    return ret;
}


key_t getKey()
{
    key_t ret = ftok(KEYFILE, 65);
    if (ret == -1)
    { 
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
    { 
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




/*
*
* tool traduzione da id (interno al sistema) a pid del S.O
* TODO: gestione cache per ottimizzare la traduzione di componenti già risolte (facoltativo)
*
*/

//traduce un id in un pid
long getPidById(const int id)
{
    long ret = -1;
    //TODO: Destro Iterare nella lista dei figli
    while(ret == -1){
        int id_processo = 1; //TODO: Destro Elemento corrente della lista
        message_t msg = {.to = id_processo, .session = sessione, .text = "TRANSLATE"};

        if(sendMessage(msg) == -1){
            printf("Errore comunicazione, riprovare\n");
            break;
        }

        message_t response = receiveMessage(getpid());
        if(response.to == -1)
            continue;
        if (response.state == 1 && strcmp(response.text, "TRANSLATE") == 0) //trovato l'id che stavo cercando
            ret = response.sender;
    }
    return ret;
}
