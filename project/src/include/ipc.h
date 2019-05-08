#ifndef _IPC_
#define _IPC_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "../include/constants.h"
#include "../include/list.h"
#define MAXMSG 20
#define NVAL 6
#define KEYFILE "progfile"

time_t sessione;
int mqid;

typedef struct msg {
    long to;
    long sender;
    time_t session;
    char text[MAXMSG];
    long vals[NVAL];
} message_t;

/////////////////////////////// WORKERS ///////////////////////////////
void doList(list_t children, const char* mode, const long responde_to);

void doLink(list_t figli, long to_clone_pid);

/*  Print struct (per debug) */
void printMsg(const message_t* msg);

/* Metodo di comodo per stampare le info del comando LIST */
void printListMessage(const message_t* msg);

//############# REQUESTS ###########

message_t buildInfoRequest(list_t figli, const long to_id);

message_t buildTranslateRequest(const long to, const int searching);

message_t buildDieRequest(list_t figli, const long to_id);

message_t buildListRequest(const long to_pid);

message_t buildSwitchRequest(list_t figli, const long to_id, char* label, char* pos);

message_t buildCloneRequest(const long to_pid);

//############# RESPONSES ###########

message_t buildInfoResponse(const long id, const short stato, const int to, const char* tipo_componente);

message_t buildSwitchResponse(const int success, const int to);

message_t buildTranslateResponse(const long id, const int searching, const int to);

message_t buildDieResponse(const long to);

message_t buildListResponse(const long to_pid, const char* nome, const short stato, const long livello, const short stop, const short id);

message_t buildCloneResponse(const long to_pid, const char* type, long vals[]);

//############# IPC ###########

short int sendMessage(const message_t const* msg);

int receiveMessage(const long reader, message_t* msg);

/* Inizializza i componenti per comunicare */
void ipcInit();

/* Get key to create Message queue */
key_t getKey();

/* Get message queue */
int getMq();

/* Close message queue */
void closeMq(const int id);

/* Traduzione id-pid */
long getPidById(list_t figli, const int id);

/* Salva nel file con nome di sessione la tipologia del messaggio */
int printLog(const message_t msg);

#endif
