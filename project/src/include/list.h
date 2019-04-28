#ifndef _LIST_
#define _LIST_

typedef struct nodo {
    int value;
    struct nodo *next;
} node;

typedef node **list;

/* Inizializza la lista */
list list_init();

/*  Aggiunge "value" all'inizio della lista */
int list_push(list l, int value);

/*  Rimuove il primo elemento con valore "value" dalla lista */
int list_remove(list l, int value);

/*  Stampa la lista (per debug) */
void list_print(list l);

#endif
