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
#define MAXMSG 255
#define NVAL 6
#define KEYFILE "progfile"

time_t session;
int mqid;

typedef struct msg {
    int to;
    int sender;
    short type;
    time_t session;
    char text[MAXMSG];
    int vals[NVAL];
} message_t;

/////////////////////////////// WORKERS ///////////////////////////////
void doLink(list_t children, int to_clone_pid, int sender, const char* base_dir);
void doListController(list_t children);

/////////////////////////////// REQUESTS ///////////////////////////////
message_t buildRequest(int to_pid, short msg_type);
message_t buildInfoRequest(int to_pid);
message_t buildTranslateRequest(int to_pid, int search_id);
message_t buildDeleteRequest(int to_pid);
message_t buildListRequest(int to_pid);
message_t buildSwitchRequest(int to_pid, char* label, char* pos);
message_t buildCloneRequest(int to_pid);
message_t buildGetChildRequest(int to_pid);
message_t buildLinkRequest(int to_pid, int to_clone_pid);

/////////////////////////////// RESPONSES ///////////////////////////////
message_t buildResponse(int to_pid, short msg_type);
message_t buildInfoResponse(int to_pid, const char* tipo_componente);
message_t buildSwitchResponse(int to_pid, short success);
message_t buildTranslateResponse(int to_pid, int pid_found);
message_t buildTranslateResponseControl(int sender, int my_id, int search, list_t children);
message_t buildDeleteResponse(int to_pid);
message_t buildListResponse(int to_pid, int id, const char* text, int lv, short state, short stop);
message_t buildCloneResponse(int to_pid, const char* component_type, const int vals[]);
message_t buildGetChildResponse(int to_pid, int child_pid);
message_t buildLinkResponse(int to_pid, short success);
message_t buildBusyResponse(int to_pid);
message_t buildDieMessage(int to_pid);

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
int getPidById(list_t figli, int id);

//////////////////////////////// DEBUG /////////////////////////////////////
/*  Print struct (per debug) */
void printMsg(const message_t* msg);

/* Salva nel file con nome di sessione la tipologia del messaggio */
int printLog(const message_t* msg);
#endif
