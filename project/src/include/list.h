#ifndef _LIST_
#define _LIST_
#include <stdlib.h>  //tmp da Steve
#define MAXTEXT 255
#define NVAL 10
#define KEYFILE "progfile"
//#include "../include/ipc.h" tmp da Steve

typedef struct node {
    int value;
    struct node *next;
} node_t;

typedef node_t **list_t;

/* Inizializza la lista */
list_t listInit();

/* Dealloca la lista */
void listDestroy(list_t l);

/*  Aggiunge "value" all'inizio della lista */
int listPush(list_t l, int value);

/*  Rimuove il primo elemento con valore "value" dalla lista */
int listRemove(list_t l, int value);

/*  Ritorna 1 se la lista contiene il valore, 0 altrimenti */
int listContains(list_t l, int value);

/*  Ritorna il numero di elementi nella lista */
int listCount(list_t l);

/*  Ritorna 1 se la lista è vuota, 0 altrimenti */
int listEmpty(list_t l);

/*  Stampa la lista (per debug) */
void listPrint(list_t l);


//tmp da Steve
typedef struct msg {
    long to;
    int sender;
    short type;
    time_t session;
    char text[MAXTEXT];
    int vals[NVAL];
} message_t;

typedef struct node_msg {
    message_t value;
    struct node_msg *next;
} node_msg_t;

typedef node_t **list_msg_t;

/* Inizializza la lista */
list_msg_t listInit();

/* Dealloca la lista */
void listMsgDestroy(list_msg_t l);

/*  Aggiunge "value" all'inizio della lista */
int listMsgPush(list_msg_t l, message_t m);

/*  Rimuove il primo elemento con valore "value" dalla lista */
int listMsgRemove(list_msg_t l, message_t m);

/*  Ritorna 1 se la lista contiene il valore, 0 altrimenti */
int listMsgContains(list_msg_t l, message_t m);

/*  Ritorna il numero di elementi nella lista */
int listMsgCount(list_msg_t l);

/*  Ritorna 1 se la lista è vuota, 0 altrimenti */
int listMsgEmpty(list_msg_t l);

/*  Stampa la lista (per debug) */
void listMsgPrint(list_msg_t l);

#endif
