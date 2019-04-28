#ifndef _LIST_
#define _LIST_

typedef struct node {
    int value;
    struct node *next;
} node_t;

typedef node_t **list_t;

/* Inizializza la lista */
list_t list_init();

/*  Aggiunge "value" all'inizio della lista */
int list_push(list_t l, int value);

/*  Rimuove il primo elemento con valore "value" dalla lista */
int list_remove(list_t l, int value);

/*  Stampa la lista (per debug) */
void list_print(list_t l);

#endif
