#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ipc.h"  //TODO: Destro linkare libreria ipc.c
#include "../include/list.h"

#define MAX_DEVICE_NAME_LENGTH 20

list_t children;
int next_id;
char *base_dir;

/*  Inizializza le variabili del controller   */
void controller_init(char *file) {
    children = list_init();
    ipc_init();  //inizializzo componenti comunicazione
    next_id = 1;
    //  Uso il percorso relativo al workspace, preso da argv[0] per trovare gli altri eseguibili per i device.
    //  Rimuovo il nome del file dal percorso
    base_dir = malloc(sizeof(file) + MAX_DEVICE_NAME_LENGTH);
    strcpy(base_dir, file);
    char *last_slash = strrchr(base_dir, '/');
    if (last_slash) *(last_slash + 1) = '\0';
}

/*  Dealloca il controller  */
void controller_destroy() {
    list_destroy(children);
    free(base_dir);
}

/**************************************** LIST ********************************************/
void list_devices() {
    printf("Elenco componenti:\n\n");
    printf("Controller with %d direct connected components\n", 1 );
    doList(children, "CONTROLLER", getpid());  //eseguo il comando LIST con comportamento  controller
}

/**************************************** ADD ********************************************/
/*  (private) Aggiunge un dispositivo al controller in base al tipo specificato   */
int _add_device(char *file) {
    int pid = fork();
    /*  Processo figlio */
    if (pid == 0) {
        char str_id[10];
        strcat(base_dir, file);               //  Genero il path dell'eseguibile
        snprintf(str_id, 10, "%d", next_id);  //  Converto id in stringa
        char *args[] = {base_dir, str_id, NULL};
        return execvp(args[0], args);
    }
    /*  Processo padre */
    else {
        if (pid != -1) {
            next_id++;
            list_push(children, pid);
        }
        return pid;
    }
}

int add_bulb() {
    return _add_device("bulb");
}

int add_fridge() {
    return _add_device("fridge");
}

int add_window() {
    return _add_device("window");
}

int add_hub() {
    return _add_device("hub");
}

int add_timer() {
    return _add_device("timer");
}

/**************************************** DEL ********************************************/
int del_device(char *id) {
    printf("Elimino  %s ...\n", id);

    int id_da_cercare = atoi(id);
    message_t request = buildDieRequest(children, id_da_cercare);
    if (sendMessage(&request) == -1)
        printf("Errore comunicazione, riprova");

    message_t response;
    if (receiveMessage(getpid(), &response) != -1) {
        if (strcmp(response.text, MSG_DELETE_RESPONSE) == 0) {
            printf("%d died", id_da_cercare);
            list_remove(children, id_da_cercare);
        } else
            printf("error dying %s", response.text);
    }
}

/**************************************** LINK ********************************************/
int link_devices(char *id1, char *id2) {
    printf("TODO: link device %s to %s\n", id1, id2);
}

/**************************************** SWITCH ********************************************/
int switch_device(char *id, char *label, char *pos) {
    printf("TODO: switch id: %s, label: %s, pos: %s\n", id, label, pos);
}

/**************************************** INFO ********************************************/
//TODO: Distinguere info in base al tipo di componente
//TODO: Se id non è un numero valido dare errore
void info_device(char *id) {
    printf("Ottengo le info da %s ...\n", id);
    int id_da_cercare = atoi(id);
    message_t request = buildInfoRequest(children, id_da_cercare);
    if (sendMessage(&request) == -1)
        printf("Errore comunicazione, riprova");
    message_t response;
    if (receiveMessage(getpid(), &response) == -1) {
        perror("Errore info device");
    } else {
        printf("%s ", response.text);
        if (response.value6 == 1)
            printf("accesa ");
        else
            printf("spenta ");
        printf(". Tempo di funzionamento = %ld", response.value1);
    }
}