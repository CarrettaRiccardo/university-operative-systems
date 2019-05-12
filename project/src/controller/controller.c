#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ipc.h"
#include "../include/list.h"
#include "./shell.c"

int id;
list_t children;
int next_id;
char *base_dir;

/*  Inizializza le variabili del controller   */
void controllerInit(char *file) {
    children = listInit();
    ipcInit();  //inizializzo componenti comunicazione
    id = 0;
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
    printf("(0) controller\n");
    node_t *p = *children;
    while (p != NULL) {
        message_t request = buildListRequest(p->value);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error list request");
        } else {
            do {
                if (receiveMessage(&response) == -1) {
                    perror("Error list response");
                } else {
                    int i;
                    for (i = 0; i < response.vals[LIST_VAL_LEVEL] + 1; i++) printf("    ");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)
                    printf("|-(%d) %s\n", response.vals[LIST_VAL_ID], response.text);
                }
            } while (response.vals[LIST_VAL_STOP] != 1);
        }
        p = p->next;
    }
}

/**************************************** ADD ********************************************/
/*  Aggiunge un dispositivo al controller in base al tipo specificato   */
int addDevice(char *device) {
    int pid = fork();
    /*  Processo figlio */
    if (pid == 0) {
        char str_id[10];
        strcat(base_dir, device);             //  Genero il path dell'eseguibile
        snprintf(str_id, 10, "%d", next_id);  //  Converto id in stringa
        char *args[] = {base_dir, str_id, NULL};
        return execvp(args[0], args);
    }
    /*  Processo padre */
    else {
        if (pid == -1) return -1;
        next_id++;
        listPush(children, pid);
        return next_id - 1;
    }
}

/**************************************** DEL ********************************************/
void delDevice(int id) {
    int pid = getPidById(children, id);
    if (pid == -1) {
        printf("Error: device with id %d not found\n", id);
        return;
    }
    message_t request = buildDeleteRequest(pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error deleting device request");
    } else if (receiveMessage(&response) == -1) {
        perror("Error deleting device response");
    } else if (response.type == DELETE_MSG_TYPE) {
        printf("Device %d deleted\n", id);
        listRemove(children, pid);
    } else {
        printf("Error deleting %d: %s\n", id, response.text);
    }
}

/**************************************** LINK ********************************************/
void linkDevices(int id1, int id2) {
    int src = getPidById(children, id1);
    if (src == -1) {
        printf("Error: device with id %d not found\n", id1);
        return;
    }
    int dest = getPidById(children, id2);
    if (dest == -1) {
        printf("Error: device with id %d not found\n", id2);
        return;
    }

    message_t request = buildLinkRequest(dest, src);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error linking devices request");
    } else if (receiveMessage(&response) == -1) {
        perror("Error linking devices response");
    } else if (response.vals[LINK_VAL_SUCCESS] == -1) {
        printf("Error: the device with id %d is not a control device\n", id2);
    } else if (response.vals[LINK_VAL_SUCCESS] == LINK_MAX_CHILD) {
        printf("Error: the device with id %d already has a child\n", id2);
    } else {
        //  Killo il processo src già clonato
        request = buildDeleteRequest(src);
        if (sendMessage(&request) == -1) {
            perror("Error deleting device request");
        } else if (receiveMessage(&response) == -1) {
            perror("Error deleting device response");
        } else if (response.type == DELETE_MSG_TYPE) {
            listRemove(children, src);
            printf("Device %d linked to %d\n", id1, id2);
        }
    }
}

/**************************************** SWITCH ********************************************/
int switchDevice(int id, char *label, char *pos) {
    printf("Modifico l'interruttore %s di %d su %s ...\n", label, id, pos);
    int pid = getPidById(children, id);
    if (pid == -1) {
        printf("Error: device with id %d not found\n", id);
        return;
    }
    message_t request = buildSwitchRequest(pid, label, pos);
    message_t response;

    // Se i parametri creano dei valori validi
    if (request.vals[SWITCH_VAL_LABEL] != __INT_MAX__ && request.vals[SWITCH_VAL_POS] != __INT_MAX__) {
        if (sendMessage(&request) == -1)
            printf("Errore comunicazione, riprova\n");

        if (receiveMessage(&response) == -1) {
            perror("Errore switch\n");
        } else {
            if (response.vals[SWITCH_VAL_SUCCESS] != -1) {
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
void infoDevice(int id) {
    if (id == 0) {
        printf("Device type: controller, registers: num = %d\n", listCount(children));
    } else {
        int pid = getPidById(children, id);
        if (pid == -1) {
            printf("Error: device with id %d not found\n", id);
            return;
        }
        message_t request = buildInfoRequest(pid);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error info request");
        } else if (receiveMessage(&response) == -1) {
            perror("Errore info response");
        } else {
            //  Stampo il testo ricevuto dal dispositivo
            printf("Device type: %s\n", response.text);
        }
    }
}
