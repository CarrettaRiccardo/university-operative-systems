#ifndef _LIST_
#define _LIST_

typedef struct node {
    long value;
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

#endif
