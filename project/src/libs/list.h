#ifndef _LIST_
#define _LIST_

typedef struct Tdato {
    int index;
    float value;
};

typedef struct Tnodo {
    Tdato dato;
    Tnodo* next;
};

typedef struct Tlista {  //solo per esercizi aggiuntivi
    Tnodo* head;
    //posso insere le fnzioni fatte prima e metterli come metodi qui dentro
};
typedef Tdato Dato;
typedef Tnodo Nodo;
typedef Tnodo* Nodoptr;

int casuale(int min, int max);
float fcasuale(int min, int max);

Nodoptr insert_first(Nodoptr s, Dato d);
Nodoptr insert_last(Nodoptr s, Dato d);
void stampa(Nodoptr s);
Nodoptr remove_first(Nodoptr s);
Nodoptr remove_last(Nodoptr s);

#endif
