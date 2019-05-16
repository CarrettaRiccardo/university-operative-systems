#ifndef _LIST_
#define _LIST_
#include <stdlib.h>
#include "../include/constants.h"

#define MAXTEXT 255
#define NVAL 10

typedef struct msg {
    long to;
    int sender;
    short type;
    time_t session;
    char text[MAXTEXT];
    int vals[NVAL];
} message_t;

typedef struct node {
    void *value;
    struct node *next;
} node_t;

struct list {
    struct node *head;
    int (*equal)(const void *, const void *);
};

typedef struct list *list_t;

/* Inizializza una lista (generale) */
list_t listInit(int (*equal)(const void *, const void *));

/* Inizializza una lista di int */
list_t listIntInit();

/* Inizializza una lista di messsage_t */
list_t listMsgInit();

/* Dealloca la lista */
void listDestroy(list_t l);

/*  Aggiunge "value" alla fine della lista */
int listPushBack(list_t l, void *value, size_t size);

/*  Rimuove il primo elemento con valore "value" dalla lista */
int listRemove(list_t l, void *value);

/*  Ritorna 1 se la lista contiene il valore, 0 altrimenti */
int listContains(list_t l, void *value);

/*  Ritorna il numero di elementi nella lista */
int listCount(list_t l);

/*  Ritorna 1 se la lista è vuota, 0 altrimenti */
int listEmpty(list_t l);

/*  Stampa la lista (per debug) */
void listIntPrint(list_t l);

#endif
