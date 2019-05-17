#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ipc.h"
#include "../include/list.h"
#include "./manual_shell.c"

int solved_pid;

/************************************ REQUEST GET PID BY ID ***********************************/
/* Chiede al controller il pid collegato al device id                                         */
/**********************************************************************************************/
int requestGetPidById(int id) {
    if (sendGetPidByIdSignal(controller_pid, id) < 0) {
        perror("Error: cannot send request getPidByIdSignal");
        return -1;
    } else {
        pause();
        return solved_pid;
    }
}

static void getPidByIdSignalHandler(int sig, siginfo_t *siginfo, void *context) {
    solved_pid = siginfo->si_value.sival_int;
}

/**************************************** INIT ********************************************/
/* Inizializzazione valori cmanual                                                        */
/******************************************************************************************/
void controllerInit() {
    // Registrazione handler per risposta getPidById dal controller
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_sigaction = getPidByIdSignalHandler;
    sig.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &sig, NULL) < 0) {
        perror("Error: cannot register SIGUSR1 handler");
    }
    solved_pid = -1;
}

/*  Dealloca manual  */
void controllerDestroy() {}
/**************************************** DEL ********************************************/
/* Elimina un dispositivo in base all'id specificato                                     */
/*****************************************************************************************/
void delDevice(int id) {
    if (id == 0) {
        printf("Error: cannot delete the controller\n");
        return;
    }

    int pid = requestGetPidById(id);
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
    int src = requestGetPidById(id1);
    if (src == -1) {
        printf("Error: device with id %d not found\n", id1);
        return;
    }

    message_t request, response;
    if (id2 != 0) {
        // Risolvo l'id2 in un PID valido
        int dest = requestGetPidById(id2);
        if (dest == -1) {
            printf("Error: device with id %d not found or not connected to the controller\n", id2);
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
    int pid = requestGetPidById(id);
    if (pid == -1) {
        printf("Error: device with id %d not found or not connected to the controller\n", id);
        return;
    }
    int label_val = __INT_MAX__;  // 0 = interruttore (generico), 1 = termostato
    int pos_val = __INT_MAX__;    // 0 = spento, 1 = acceso; x = valore termostato (°C)
    // Map delle label (char*) in valori (int) per poterli inserire in un messaggio
    if (strcmp(label, LABEL_LIGHT) == 0) {
        label_val = LABEL_LIGHT_VALUE;  // 1 = interruttore (luce)
    } else if (strcmp(label, LABEL_OPEN) == 0) {
        label_val = LABEL_OPEN_VALUE;  // 2 = interruttore (apri/chiudi)
    } else if (strcmp(label, LABEL_TERM) == 0) {
        label_val = LABEL_TERM_VALUE;  // 4 = termostato
    } else if (strcmp(label, LABEL_ALL) == 0) {
        label_val = LABEL_ALL_VALUE;  // 8 = all (generico)
    }

    // Map valore pos (char*) in valori (int) per poterli inserire in un messaggio
    if (label_val == LABEL_LIGHT_VALUE || label_val == LABEL_OPEN_VALUE || label_val == LABEL_ALL_VALUE) {
        // Se è un interrutore on/off
        if (strcmp(pos, SWITCH_POS_OFF) == 0) {
            pos_val = SWITCH_POS_OFF_VALUE;  // 0 = spento/chiuso
        } else if (strcmp(pos, SWITCH_POS_ON) == 0) {
            pos_val = SWITCH_POS_ON_VALUE;  // 1 = acceso/aperto
        }
    }

    // Se i parametri creano dei valori validi
    if (label_val == __INT_MAX__) {
        printf("Error: invalid 'label' value \"%s\"\n", label);
        return;
    } else if (pos_val == __INT_MAX__) {
        printf("Error: invalid 'pos' value \"%s\"\n", pos);
        return;
    } else {
        message_t request = buildSwitchRequest(pid, label_val, pos_val);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error switch request");
        } else if (receiveMessage(&response) == -1) {
            perror("Error switch response");
        } else {
            if (response.vals[SWITCH_VAL_SUCCESS] != -1) {
                printf("Switch executed\n");
            } else {
                printf("Error: switch not executed\n");
            }
        }
    }
}

/**************************************** SET ***********************************************/
/* Cambia lo stato del "register" del dispositivo "id" al valore "val"           */
/********************************************************************************************/
int setDevice(int id, char *label, char *val) {
    int pid = requestGetPidById(id);
    if (pid == -1) {
        printf("Error: device with id %d not found\n", id);
        return;
    }
    if (pid == -1) {
        printf("Error: device with id %d not found or not connected to the controller\n", id);
        return;
    }
    int label_val = __INT_MAX__;  // 0 = interruttore (generico), 1 = termostato
    int pos_val = __INT_MAX__;    // 0 = spento, 1 = acceso; x = valore termostato (°C)
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
        if (label_val == __INT_MAX__) {
            printf("Error: invalid 'register' value \"%s\"\n", label);
            return;
        } else if (pos_val == __INT_MAX__) {
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
    int pid = requestGetPidById(id);
    if (pid == -1) {
        printf("Error: device with id %d not found or not connected to the controller\n", id);
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
                for (i = 0; i < response.vals[INFO_VAL_LEVEL]; i++) printf("    |-");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)
                printf("(%d) %s\n", response.vals[INFO_VAL_ID], response.text);
            }
        } while (response.vals[INFO_VAL_STOP] != 1);
    }
}
