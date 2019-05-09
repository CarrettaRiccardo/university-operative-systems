#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ipc.h"
#include "../include/list.h"

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
// Metodo di comodo per stampare le Info da mostrare nel comando LIST
void printListMessage(const message_t *msg) {
    int i;
    for (i = 0; i < msg->vals[LIST_VAL_LEVEL]; i++) printf("    ");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)
    printf("| <%ld> %s ", msg->vals[LIST_VAL_ID], msg->text);
    if (strcmp(msg->text, BULB) == 0) {
        switch (msg->vals[LIST_VAL_STATE]) {
            case 0: printf(" off\n"); break;
            case 1: printf(" on\n"); break;
            case 2: printf(" off (override)\n"); break;
            case 3: printf(" on (override)\n"); break;
        }
    } else {
        switch (msg->vals[LIST_VAL_STATE]) {
            case 0: printf(" close\n"); break;
            case 1: printf(" open\n"); break;
            case 2: printf(" close (override)\n"); break;
            case 3: printf(" open (override)\n"); break;
        }
    }
}

void listDevices() {
    printf("<0> controller with %d children\n", listCount(children));
    node_t *p = *children;
    while (p != NULL) {
        long son = p->value;

        message_t request = buildListRequest(son);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error list request");
        } else {
            do {
                if (receiveMessage(&response) == -1) {
                    perror("Error list response");
                } else {
                    printListMessage(&response);
                }
            } while (response.vals[LIST_VAL_STOP] != 1);
        }
        p = p->next;
    }
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
        if (pid == -1) return -1;
        next_id++;
        listPush(children, pid);
        return next_id - 1;
    }
}

/**************************************** DEL ********************************************/
void delDevice(char *id) {
    long pid = getPidById(children, atoi(id));
    if (pid == -1) {
        printf("Error: device with id %s not found\n", id);
        return;
    }
    message_t request = buildDeleteRequest(pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error deleting device request");
    } else if (receiveMessage(&response) == -1) {
        perror("Error deleting device response");
    } else if (response.type == DELETE_MSG_TYPE) {
        printf("Device %s deleted\n", id);
        listRemove(children, pid);
    } else {
        printf("Error deleting %s: %s\n", id, response.text);
    }
}

/**************************************** LINK ********************************************/
void linkDevices(char *id1, char *id2) {
    long src = getPidById(children, atoi(id1));
    if (src == -1) {
        printf("Error: device with id %s not found\n", id1);
        return;
    }
    long dest = getPidById(children, atoi(id2));
    if (dest == -1) {
        printf("Error: device with id %s not found\n", id2);
        return;
    }

    message_t request = buildLinkRequest(dest, src);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error linking devices request");
    } else if (receiveMessage(&response) == -1) {
        perror("Error linking devices response");
    } else if (response.vals[LINK_VAL_SUCCESS] == -1) {
        printf("Error: the device with id %s is not a control device\n", id2);
    } else {
        //  Killo il processo src già clonato
        request = buildDeleteRequest(src);
        if (sendMessage(&request) == -1) {
            perror("Error deleting device request");
        } else if (receiveMessage(&response) == -1) {
            perror("Error deleting device response");
        } else if (response.type == DELETE_MSG_TYPE) {
            listRemove(children, src);
            printf("Device %s linked to %s\n", id1, id2);
        }
    }
}

/**************************************** SWITCH ********************************************/
int switchDevice(char *id, char *label, char *pos) {
    printf("Modifico l'interruttore %s di %s su %s ...\n", label, id, pos);
    long pid = getPidById(children, atoi(id));
    if (pid == -1) {
        printf("Error: device with id %s not found\n", id);
        return;
    }
    message_t request = buildSwitchRequest(pid, label, pos);
    message_t response;

    // Se i parametri creano dei valori validi
    if (request.vals[SWITCH_VAL_LABEL] != __LONG_MAX__ && request.vals[SWITCH_VAL_POS] != __LONG_MAX__) {
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
void infoDevice(char *id) {
    long pid = getPidById(children, atoi(id));
    if (pid == -1) {
        printf("Error: device with id %s not found\n", id);
        return;
    }
    message_t request = buildInfoRequest(pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error info request");
    } else if (receiveMessage(&response) == -1) {
        perror("Errore info response");
    } else {
        printf("Type: %s, state: ", response.text);  //stampo il nome componente
        if (strcmp(response.text, BULB) == 0) {
            if (response.vals[INFO_VAL_STATE] == SWITCH_POS_ON_VALUE)
                printf("on");
            else
                printf("off");
            printf(", work time (time set to on) = %ld\n", response.vals[1]);
        } else if (strcmp(response.text, WINDOW) == 0) {
            if (response.vals[INFO_VAL_STATE] == SWITCH_POS_ON_VALUE)
                printf("opened");
            else
                printf("closed");
            printf(", total opened time = %ld\n", response.vals[1]);
        } else if (strcmp(response.text, FRIDGE) == 0) {
            if (response.vals[INFO_VAL_STATE] == SWITCH_POS_ON_VALUE)
                printf("opened");
            else
                printf("closed");
            printf(", total opened time = %ld", response.vals[1]);
            printf(", delay = %ld", response.vals[2]);
            printf(", temperature = %ld", response.vals[3]);
            printf(", percent filled = %ld\n", response.vals[4]);
        } else if (strcmp(response.text, HUB) == 0) {
            // TODO
        } else if (strcmp(response.text, TIMER) == 0) {
            // TODO
        }
    }
}
