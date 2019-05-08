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
void controllerInit(char *file) {
    children = listInit();
    ipcInit();  //inizializzo componenti comunicazione
    next_id = 1;
    //  Uso il percorso relativo al workspace, preso da argv[0] per trovare gli altri eseguibili per i device.
    //  Rimuovo il nome del file dal percorso
    base_dir = malloc(sizeof(file) + MAX_DEVICE_NAME_LENGTH);
    strcpy(base_dir, file);
    char *last_slash = strrchr(base_dir, '/');
    if (last_slash) *(last_slash + 1) = '\0';
}

/*  Dealloca il controller  */
void controllerDestroy() {
    listDestroy(children);
    free(base_dir);
}

/**************************************** LIST ********************************************/

void listDevices() {
    printf("Elenco componenti:\n");
    printf("Controller with %d direct connected components:\n", listCount(children));
    doList(children, "CONTROLLER", getpid());  //eseguo il comando LIST con comportamento  controller
}

/**************************************** ADD ********************************************/
/*  Aggiunge un dispositivo al controller in base al tipo specificato   */
int addDevice(char *file) {
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
            listPush(children, pid);
        }
        return pid;
    }
}

/**************************************** DEL ********************************************/
int delDevice(char *id) {
    printf("Elimino  %s ...\n", id);

    int id_da_cercare = atoi(id);
    message_t request = buildDieRequest(children, id_da_cercare);
    if (sendMessage(&request) == -1)
        printf("Errore comunicazione, riprova");

    int pid_trovato = request.to;  //contiene la traduzione id-pid risolta
    message_t response;
    if (receiveMessage(getpid(), &response) != -1) {
        if (strcmp(response.text, MSG_DELETE_RESPONSE) == 0) {
            printf("%d died\n", id_da_cercare);
            listRemove(children, pid_trovato);
        } else
            printf("error dying %s\n", response.text);
    }
}

/**************************************** LINK ********************************************/
void linkDevices(char *id1, char *id2) {
    printf("TODO: link device %s to %s\n", id1, id2);
    long src = getPidById(children, atoi(id1));
    long dest = getPidById(children, atoi(id2));
    if (src == -1) printf("Error: id %s not found\n", id1);
    if (dest == -1) printf("Error: id %s not found\n", id2);
    printf("Got pid %ld to %ld\n", src, dest);
}

/**************************************** SWITCH ********************************************/
int switchDevice(char *id, char *label, char *pos) {
    printf("Modifico l'interruttore %s di %s su %s ...\n", label, id, pos);
    int id_da_cercare = atoi(id);
    message_t request = buildSwitchRequest(children, id_da_cercare, label, pos);
    // se i parametri creano dei valori validi
    if (request.vals[0] != -1 && request.vals[1] != -1) {
        if (sendMessage(&request) == -1)
            printf("Errore comunicazione, riprova\n");
        message_t response;
        if (receiveMessage(getpid(), &response) == -1) {
            perror("Errore switch\n");
        } else {
            if (response.vals[5] != -1) {
                printf("Modifica effettuata con successo\n");
            } else {
                printf("Errore nella modifica\n");
            }
        }
    } else {
        perror("Parametri non corretti o coerenti\n");
    }
}

/**************************************** INFO ********************************************/
//TODO: Distinguere info in base al tipo di componente
//TODO: Se id non è un numero valido dare errore
//TODO: Destro mappare i nomi dei compnenti con le costanti
void infoDevice(char *id) {
    printf("Ottengo le info da %s ...\n", id);
    int id_da_cercare = atoi(id);
    message_t request = buildInfoRequest(children, id_da_cercare);
    if (sendMessage(&request) == -1) {
        printf("Errore: destinatario invalido\n");
        return;  //destinatario non trovato, non mi pongo in attesa di un suo messaggio
    }

    message_t response;
    if (receiveMessage(getpid(), &response) == -1) {
        perror("Errore info device");
        return;
    }

    printf("%s ", response.text);  //stampo il nome componente

    if (strcmp(response.text, BULB) == 0) {
        if (response.vals[5] == 1)
            printf("on");
        else
            printf("off");
        printf(". Work time (time set to on) = %ld\n", response.vals[0]);
    } else if (strcmp(response.text, HUB) == 0) {
    } else if (strcmp(response.text, TIMER) == 0) {
    }
}
