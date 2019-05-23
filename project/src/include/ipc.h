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

int mqid;

/********************************** Workers **********************************/
int doLink(list_t children, int to_clone_pid, const char* base_dir, short is_terminal);

/********************************** Requests **********************************/
message_t buildRequest(int to_pid, short msg_type);
message_t buildInfoRequest(int to_pid);
message_t buildTranslateRequest(int to_pid, int search_id);
message_t buildDeleteRequest(int to_pid);
message_t buildListRequest(int to_pid);
message_t buildSwitchRequest(int to_pid, int label, int pos);
message_t buildSetRequest(int to_pid, int reg, int value);
message_t buildCloneRequest(int to_pid);
message_t buildGetChildRequest(int to_pid);
message_t buildLinkRequest(int to_pid, int to_clone_pid);

/********************************** Responses **********************************/
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

/********************************** Send/Receive **********************************/
int sendMessage(const message_t* request);
int receiveMessage(message_t* response);

/********************************** Signals **********************************/
/* Invia la richiesta di traduzione id in pid al controller / invia il pid risolto alla shell manuale */
int sendGetPidByIdSignal(int to_pid, int id);

/********************************** Init **********************************/
/* Inizializza i componenti per comunicare */
void ipcInit(int _mqid);

/* Get message queue */
int getMq();

/* Close message queue */
void closeMq();

/* Traduzione id-pid, ritorna il pid corrispondente all'id passato o -1 se non esiste */
int getPidById(list_t figli, int id);

/* Funzione appoggio per implementare getPidById */
int getPidByIdSingle(int to_pid, int id);
#endif
