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

time_t session;
int mqid;

typedef struct msg {
    long to;
    long sender;
    short type;
    time_t session;
    char text[MAXMSG];
    long vals[NVAL];
} message_t;

/////////////////////////////// WORKERS ///////////////////////////////
void doLink(list_t children, long to_clone_pid, long sender, const char* base_dir);
void doList(long to_pid, list_t children);

/////////////////////////////// REQUESTS ///////////////////////////////
message_t buildRequest(long to_pid, short msg_type);
message_t buildInfoRequest(long to_pid);
message_t buildTranslateRequest(long to_pid, int search_id);
message_t buildDeleteRequest(long to_pid);
message_t buildListRequest(long to_pid);
message_t buildSwitchRequest(long to_pid, char* label, char* pos);
message_t buildCloneRequest(long to_pid);
message_t buildLinkRequest(long to_pid, long to_clone_pid);

/////////////////////////////// RESPONSES ///////////////////////////////
message_t buildResponse(long to_pid, short msg_type);
message_t buildInfoResponse(long to_pid, const char* tipo_componente);
message_t buildSwitchResponse(long to_pid, short success);
message_t buildTranslateResponse(long to_pid, short found);
message_t buildDeleteResponse(long to_pid);
message_t buildListResponse(long to_pid, const char* component_type, int id, int lv, short state, short stop);
message_t buildCloneResponse(long to_pid, const char* component_type, const long vals[]);
message_t buildLinkResponse(long to_pid, short success);

/////////////////////////////// IPC ///////////////////////////////
int sendMessage(const message_t* request);
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
