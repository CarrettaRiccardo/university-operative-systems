#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ipc.h"
#include "../include/list.h"
#include "./shell.c"

int id, next_id;
list_t disconnected_children;
list_t connected_children;
char *base_dir;

/* Handler segnale eliminazione figli */
void sigchldHandler(int signum) {
    int pid;
    do {
        pid = waitpid(-1, NULL, WNOHANG);
        listRemove(disconnected_children, &pid);
        listRemove(connected_children, &pid);
    } while (pid != -1 && pid != 0);
}

/* Handler richiesta traduzione figlio */
void getPidByIdSignalHandler(int sig, siginfo_t *siginfo, void *context) {
    int to_solve_id = siginfo->si_value.sival_int;
    int pid = getPidById(connected_children, to_solve_id);
    if (sendGetPidByIdSignal(siginfo->si_pid, pid) < 0) {
        perror("Error: cannot send response getPidByIdSignal");
    }
}

/**************************************** INIT ********************************************/
/* Inizializzazione valori controller                                                     */
/******************************************************************************************/
void controllerInit(char *file) {
    // Registrazione handler per risposta getPidById dal controller
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_sigaction = getPidByIdSignalHandler;
    sig.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &sig, NULL) < 0) {
        perror("Error: cannot register SIGUSR1 handler");
    }

    // Registrazione handler per signal terminazione figli
    signal(SIGCHLD, sigchldHandler);

    connected_children = listIntInit();
    disconnected_children = listIntInit();

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
    listDestroy(connected_children);
    listDestroy(disconnected_children);
    free(base_dir);
}

/**************************************** LIST ********************************************/
/* Stampa l'albero dei dispositivi con l'id e lostato                                     */
/******************************************************************************************/
void listDevicesInList(list_t children, short show_tree) {
    node_t *p = children->head;
    while (p != NULL) {
        message_t request = buildListRequest(*(int *)p->value);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error list request");
        } else {
            do {
                if (receiveMessage(&response) == -1) {
                    perror("Error list response");
                } else {
                    int i;
                    for (i = 0; i < response.vals[LIST_VAL_LEVEL] + show_tree - 1; i++) printf("    ");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)
                    if (show_tree == 1 || response.vals[LIST_VAL_LEVEL] > 0) printf(C_CYAN " └──" C_WHITE);
                    printf(CB_CYAN "(%d)" C_WHITE " %s\n", response.vals[LIST_VAL_ID], response.text);
                }
            } while (response.vals[LIST_VAL_STOP] != 1);
        }
        p = p->next;
    }
}

void listDevices() {
    listDevicesInList(disconnected_children, 0);
    printf(CB_CYAN "(0)" CB_WHITE " controller" C_WHITE "\n");
    listDevicesInList(connected_children, 1);
}

/**************************************** ADD ********************************************/
/* Aggiunge un dispositivo al controller in base al tipo specificato                     */
/*****************************************************************************************/
int addDevice(char *device) {
    int pid = fork();
    /*  Processo figlio */
    if (pid == 0) {
        char mqid_str[20], id_str[20];
        strcat(base_dir, device);             //  Genero il path dell'eseguibile
        snprintf(mqid_str, 20, "%d", mqid);   //  Converto mqid in stringa
        snprintf(id_str, 20, "%d", next_id);  //  Converto id in stringa
        char *args[] = {base_dir, mqid_str, id_str, NULL};
        return execvp(args[0], args);
    }
    /*  Processo padre */
    else {
        if (pid == -1) return -1;
        next_id++;
        listPushBack(disconnected_children, &pid, sizeof(int));
        return next_id - 1;
    }
}

/**************************************** DEL ********************************************/
/* Elimina un dispositivo in base all'id specificato                                     */
/*****************************************************************************************/
void delDevice(int id) {
    if (id == 0) {
        printf("Error: cannot delete the controller\n");
        return;
    }

    int pid = getPidById(disconnected_children, id);
    if (pid == -1) pid = getPidById(connected_children, id);
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
    } else {
        printf("Device %d deleted\n", id);
    }
}

/**************************************** LINK ********************************************/
/* Effettua il link di id1 a id2                                                          */
/******************************************************************************************/
void linkDevices(int id1, int id2) {
    if (id1 == 0) {
        printf("Error: cannot connect the controller to other devices\n");
        return;
    }

    // Risolvo l'id1 in un PID valido
    int src = getPidById(disconnected_children, id1);
    if (src == -1) src = getPidById(connected_children, id1);
    if (src == -1) {
        printf("Error: device with id %d not found\n", id1);
        return;
    }

    message_t request, response;
    if (id2 == 0) {
        // Connetto src al controller (id = 0)
        doLink(connected_children, src, base_dir);
        receiveMessage(NULL);  //  Attendo una conferma dal figlio clonato.
    } else {
        // Risolvo l'id2 in un PID valido
        int dest = getPidById(disconnected_children, id2);
        if (dest == -1) dest = getPidById(connected_children, id2);
        if (dest == -1) {
            printf("Error: device with id %d not found\n", id2);
            return;
        }

        // Se il src contiene dest tra i sui figli, sto creando un ciclo.
        request = buildTranslateRequest(src, id2);
        if (sendMessage(&request) == -1) {
            perror("Error get pid by id request");
            return;
        } else if (receiveMessage(&response) == -1) {
            perror("Error get pid by id response");
            return;
        } else if (response.vals[TRANSLATE_VAL_ID] > 0) {
            printf("Error: cycle identified, %d is a child of %d\n", id2, id1);
            return;
        }

        // Invio la richiesta di link a dest con il PID di src
        request = buildLinkRequest(dest, src);
        if (sendMessage(&request) == -1) {
            perror("Error linking devices request");
            return;
        } else if (receiveMessage(&response) == -1) {
            perror("Error linking devices response");
            return;
        } else if (response.vals[LINK_VAL_SUCCESS] == LINK_ERROR_NOT_CONTROL) {
            printf("Error: the device with id %d is not a control device\n", id2);
            return;
        } else if (response.vals[LINK_VAL_SUCCESS] == LINK_ERROR_MAX_CHILD) {
            printf("Error: the device with id %d already has a child\n", id2);
            return;
        }
    }
    //  Killo il processo src già clonato
    request = buildDeleteRequest(src);
    if (sendMessage(&request) == -1) {
        perror("Error deleting device request");
    } else if (receiveMessage(&response) == -1) {
        perror("Error deleting device response");
    } else {
        printf("Device %d linked to %d\n", id1, id2);
    }
}

/**************************************** SWITCH ********************************************/
/* Cambia lo stato dell'interruttore "label" del dispositivo "id" al valore "pos"           */
/********************************************************************************************/
int switchDevice(int id, char *label, char *pos) {
    int pid = getPidById(disconnected_children, id);
    if (pid == -1) pid = getPidById(connected_children, id);
    if (pid == -1) {
        printf("Error: device with id %d not found or not connected to the controller\n", id);
        return;
    }
    int label_val = INVALID_VALUE;  // 0 = interruttore (generico), 1 = termostato
    int pos_val = INVALID_VALUE;    // 0 = spento, 1 = acceso; x = valore termostato (°C)
    // Map delle label (char*) in valori (int) per poterli inserire in un messaggio
    if (strcmp(label, LABEL_LIGHT) == 0) {
        label_val = LABEL_LIGHT_VALUE;  // 1 = interruttore (luce)
    } else if (strcmp(label, LABEL_OPEN) == 0) {
        label_val = LABEL_OPEN_VALUE;  // 2 = interruttore (apri/chiudi)
    } else if (strcmp(label, LABEL_THERM) == 0) {
        label_val = LABEL_THERM_VALUE;  // 4 = termostato
    } else if (strcmp(label, LABEL_ALL) == 0) {
        label_val = LABEL_ALL_VALUE;  // 8 = all (generico)
    }

    // Map valore pos (char*) in valori (int) per poterli inserire in un messaggio
    if (label_val == LABEL_LIGHT_VALUE || label_val == LABEL_OPEN_VALUE || label_val == LABEL_ALL_VALUE) {
        // Se è un interrutore on/off
        if (strcmp(pos, SWITCH_POS_OFF_LABEL) == 0) {
            pos_val = SWITCH_POS_OFF_LABEL_VALUE;  // 0 = spento/chiuso
        } else if (strcmp(pos, SWITCH_POS_ON_LABEL) == 0) {
            pos_val = SWITCH_POS_ON_LABEL_VALUE;  // 1 = acceso/aperto
        }
    } else if (label_val == LABEL_THERM_VALUE) {
        if (isInt(pos) && atoi(pos) >= -30 && atoi(pos) <= 15) {
            pos_val = atoi(pos);
        }
    }

    // Se i parametri creano dei valori validi
    if (label_val == INVALID_VALUE) {
        printf("Error: invalid label value \"%s\"\n", label);
        return;
    } else if (pos_val == INVALID_VALUE) {
        if (label_val == LABEL_THERM_VALUE) {
            printf("Error: invalid pos value \"%s\" for label \"%s\". It must be a number between -30°C and 15°C \n", pos, label);
        } else {
            printf("Error: invalid pos value \"%s\" for label \"%s\"", pos, label);
        }

        return;
    } else {
        message_t request = buildSwitchRequest(pid, label_val, pos_val);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error switch request");
        } else if (receiveMessage(&response) == -1) {
            perror("Error switch response");
        } else {
            if (response.vals[SWITCH_VAL_SUCCESS] == SWITCH_ERROR_INVALID_VALUE) {
                printf("The label \"%s\" is not supported by the device %d\n", label, id);
            } else {
                printf("Switch executed\n");
            }
        }
    }
}

/**************************************** SET ***********************************************/
/* Cambia lo stato del "register" del dispositivo "id" al valore "val"           */
/********************************************************************************************/
int setDevice(int id, char *label, char *val) {
    int pid = getPidById(disconnected_children, id);
    if (pid == -1) pid = getPidById(connected_children, id);
    if (pid == -1) {
        printf("Error: device with id %d not found\n", id);
        return;
    }
    if (pid == -1) {
        printf("Error: device with id %d not found or not connected to the controller\n", id);
        return;
    }
    int label_val = INVALID_VALUE;  // 0 = interruttore (generico), 1 = termostato
    int pos_val = INVALID_VALUE;    // 0 = spento, 1 = acceso; x = valore termostato (°C)
    // Map delle label (char*) in valori (int) per poterli inserire in un messaggio
    if (strcmp(label, LABEL_DELAY) == 0) {
        label_val = LABEL_DELAY_VALUE;  // 1 = delay (fridge)
    } else if (strcmp(label, LABEL_BEGIN) == 0) {
        label_val = LABEL_BEGIN_VALUE;  // 2 = begin (timer)
    } else if (strcmp(label, LABEL_END) == 0) {
        label_val = LABEL_END_VALUE;  // 4 = end (timer)
    } else if (strcmp(label, LABEL_PERC) == 0) {
        label_val = LABEL_PERC_VALUE;  // 8 = perc (fridge)
    }

    if (isInt(val)) {  // E' un valore valido solo se è un numero (i register sono delay, begin o end)
        // valore del delay, di inizio o fine timer
        if (label_val == LABEL_DELAY_VALUE || label_val == LABEL_PERC_VALUE) {  // valore inserito nel delay o percentuale riempimento
            pos_val = atoi(val);
        } else if (label_val == LABEL_BEGIN_VALUE || label_val == LABEL_END_VALUE) {  // se è begin/end, il numero inserito indica quanti seconda da ORA
            pos_val = time(NULL) + atoi(val);
        }

        // Se i parametri creano dei valori validi
        if (label_val == INVALID_VALUE) {
            printf("Error: invalid 'register' value \"%s\"\n", label);
            return;
        } else if (pos_val == INVALID_VALUE) {
            printf("Error: invalid 'val' value \"%s\"\n", val);
            return;
        } else {
            message_t request = buildSetRequest(pid, label_val, pos_val);
            message_t response;
            if (sendMessage(&request) == -1) {
                perror("Error set request");
            } else if (receiveMessage(&response) == -1) {
                perror("Error set response");
            } else {
                if (response.vals[SET_VAL_SUCCESS] == -1) {
                    printf("The register \"%s\" is not supported by the device %d\n", label, id);
                } else {
                    if (response.vals[SET_VAL_SUCCESS] == SET_TIMER_STARTED_ON_SUCCESS)
                        printf("Set executed (a timer was started)\n");
                    else
                        printf("Set executed\n");
                }
            }
        }
    }
}

/**************************************** INFO ************************************************/
/* Ottiene il sottoalbero a partire dal device "id" con tutte le informazioni dei dispositivi */
/**********************************************************************************************/
void infoDevice(int id) {
    if (id == 0) {
        printf(CB_CYAN "(0)" CB_WHITE " controller" C_WHITE ", registers: num = %d\n", listCount(connected_children));
    } else {
        int pid = getPidById(disconnected_children, id);
        if (pid == -1) pid = getPidById(connected_children, id);
        if (pid == -1) {
            printf("Error: device with id %d not found\n", id);
            return;
        }
        message_t request = buildInfoRequest(pid);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error info request");
        } else {
            do {
                if (receiveMessage(&response) == -1) {
                    perror("Error info response");
                } else {
                    int i;
                    for (i = 0; i < response.vals[INFO_VAL_LEVEL]; i++) {
                        if (i == response.vals[INFO_VAL_LEVEL] - 1)
                            printf(C_CYAN " └──" C_WHITE);
                        else
                            printf("    ");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)
                    }
                    printf(CB_CYAN "(%d)" C_WHITE " %s\n", response.vals[INFO_VAL_ID], response.text);
                }
            } while (response.vals[INFO_VAL_STOP] != 1);
        }
    }
}
