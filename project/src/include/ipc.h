#ifndef _IPC_
#define _IPC_

#define MAXMSG 20
#define KEYFILE "progfile"


typedef struct msg
{
    long to;
    char text[MAXMSG];
    long value;
    long state;
    long sender;
    time_t session;
} message_t;


/* Inizializza i componenti per comunicare */
void ipc_init();

message_t buildInfoRequest(const long to_id);

message_t buildDieRequest(const long to_id);

message_t buildInfoResponse(const long id, const int valore,const short stato, const int to, const char* tipo_componente);

message_t buildTranslateResponse(const long id, const int searching, const int to);

message_t buildDieResponse(const long to);

short int sendMessage(const message_t msg);

message_t receiveMessage(const long reader);

/* Get key to create Message queue */
key_t getKey();

/* Get message queue */
int getMq();

/* Close message queue */
void closeMq(const int id);

/* Traduzione id-pid */
long getPidById(const int id);

#endif
