#ifndef _IPC_
#define _IPC_

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "../include/constants.h"
#include "../include/list.h"

time_t session;
int mqid;

/////////////////////////////// WORKERS ///////////////////////////////
void doLink(list_t children, int to_clone_pid, const char* base_dir);

/////////////////////////////// REQUESTS ///////////////////////////////
message_t buildRequest(int to_pid, short msg_type);
message_t buildInfoRequest(int to_pid);
message_t buildTranslateRequest(int to_pid, int search_id);
message_t buildDeleteRequest(int to_pid);
message_t buildListRequest(int to_pid);
message_t buildSwitchRequest(int to_pid, int label_val, int pos_val);
message_t buildSetRequest(int to_pid, int label_val, int val_val);
message_t buildCloneRequest(int to_pid);
message_t buildGetChildRequest(int to_pid);
message_t buildLinkRequest(int to_pid, int to_clone_pid);
message_t buildAddRequest(int to_pid, char* device);
message_t buildTerminalLinkRequest(int to_pid, int src, int dest);
message_t buildTerminalDeleteRequest(int home_pid, int dest);
message_t buildTerminalSwitchRequest(int home_pid, int to_pid, int label_val, int pos_val);
message_t buildTerminalInfoRequest(int to_pid, int dest);
message_t buildTerminalSetRequest(int home_pid, int id, int label, int val);

/////////////////////////////// RESPONSES ///////////////////////////////
message_t buildResponse(int to_pid, short msg_type);
message_t buildInfoResponse(int to_pid, int id, int lv, short stop);
message_t buildSwitchResponse(int to_pid, int success);
message_t buildSetResponse(int to_pid, int success);
message_t buildTranslateResponse(int to_pid, int pid_found);
message_t buildDeleteResponse(int to_pid, int response);
message_t buildListResponse(int to_pid, int id, int lv, short stop);
message_t buildCloneResponse(int to_pid, const char* component_type, int id, const int vals[], short is_control_device);
message_t buildGetChildResponse(int to_pid, int child_pid);
message_t buildLinkResponse(int to_pid, int success);
message_t buildBusyResponse(int to_pid);
message_t buildAddResponse(int to_pid, int result);


/////////////////////////////// IPC ///////////////////////////////
int sendMessage(const message_t* request);
int receiveMessage(message_t* response);

/////////////////////////////// SIGNALS ///////////////////////////////
/* Invia la richiesta di traduzione id in pid al controller / invia il pid risolto alla shell manuale */
int sendGetPidByIdSignal(int to_pid, int id);

/////////////////////////////// INIT ///////////////////////////////
/* Inizializza i componenti per comunicare */
void ipcInit(int _mqid);

/* Get key to create Message queue */
key_t getKey();

/* Get message queue */
int getMq();

/* Close message queue */
void closeMq(int id);

/* Traduzione id-pid, ritorna il pid corrispondente all'id passato o -1 se non esiste */
int getPidById(list_t figli, int id);

/* Controlla se il processo child_pid è tra i figli, percorrendo tutto l'albero */
int containsChild(list_t figli, int child_pid);

//////////////////////////////// DEBUG /////////////////////////////////////
/*  Print struct (per debug) */
void printMsg(const message_t* msg);

/* Salva nel file con nome di sessione la tipologia del messaggio */
int printLog(const message_t* msg);
#endif
