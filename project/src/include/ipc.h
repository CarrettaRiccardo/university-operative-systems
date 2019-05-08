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
void doLink(list_t figli, long to_clone_pid, long sender, const char* base_dir);
void doList(list_t children, const char* mode, long responde_to);
void printListMessage(const message_t* msg);

/////////////////////////////// REQUESTS ///////////////////////////////
message_t buildInfoRequest(list_t figli, long to_id);
message_t buildTranslateRequest(long to, int searching);
message_t buildDieRequest(long to_pid);
message_t buildListRequest(long to_pid);
message_t buildSwitchRequest(list_t figli, long to_id, char* label, char* pos);
message_t buildCloneRequest(long to_pid);
message_t buildLinkRequest(long to_pid, long to_clone_pid);

/////////////////////////////// RESPONSES ///////////////////////////////
message_t buildInfoResponse(long id, short stato, int to, const char* tipo_componente);
message_t buildSwitchResponse(int success, int to);
message_t buildTranslateResponse(long id, int searching, int to);
message_t buildDieResponse(long to_pid);
message_t buildListResponse(long to_pid, const char* nome, short stato, long livello, short stop, short id);
message_t buildCloneResponse(long to_pid, const char* type, const long vals[]);
message_t buildLinkResponse(long to_pid, int success);

/////////////////////////////// IPC ///////////////////////////////
short int sendMessage(const message_t* request);
int receiveMessage(message_t* response);

/////////////////////////////// INIT ///////////////////////////////
/* Inizializza i componenti per comunicare */
void ipcInit();

/* Get key to create Message queue */
key_t getKey();

/* Get message queue */
int getMq();

/* Close message queue */
void closeMq(int id);

/* Traduzione id-pid */
long getPidById(list_t figli, int id);

//////////////////////////////// DEBUG /////////////////////////////////////
/*  Print struct (per debug) */
void printMsg(const message_t* msg);

/* Salva nel file con nome di sessione la tipologia del messaggio */
int printLog(const message_t* msg);
#endif
