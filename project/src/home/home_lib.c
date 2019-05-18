#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ipc.h"
#include "../include/list.h"


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
        perror("Error: cannot send response to manual shell");
    }
}

/**************************************** INIT ********************************************/
/* Inizializzazione valori controller                                                     */
/******************************************************************************************/
void homeInit(char* file, char* mqid) {
    // Registrazione handler per risposta getPidById dal controller        TODO: Destro cosa significa ?
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_sigaction = getPidByIdSignalHandler;
    sig.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &sig, NULL) < 0) {
        perror("Error: cannot register SIGUSR1 handler home");
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

    int int_mqid = atoi(mqid);
    if(int_mqid <= 0)
        printf("Error: a serious problem occured. MQID not valid home_lib\n");

    ipcInit(getMq(int_mqid)); // Inizializzo componenti comunicazione
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

/*
    enable_stop : Serve per capire se inviare il flag stop. Alla prima iterazione (figli non attivi) non devo inviare tale flag, alla seconda iterazione si
*/
void listDevicesInList(int responde_to, list_t children, int enable_stop) {
    node_t *p = children->head;
    while (p != NULL) {
        message_t request = buildListRequest(*(int *)p->value);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error list sending message home_lib");
        } else {
            int stop = 0;
            do {
                if (receiveMessage(&response) == -1) {
                    perror("Error list reading response home_lib");
                } else {
                    response.to = responde_to; //cambio il destinatario del messaggio ed inoltro al display a console (terminal.c)
                    if(response.vals[LIST_VAL_STOP]){
                        if(!enable_stop)
                            response.vals[LIST_VAL_STOP] = 0;
                        stop = 1;
                    }
                    
                    sendMessage(response);
                }
            } while (!stop);
        }
        p = p->next;
    }
}

/*
    responde_to: pid processo al quale inoltrare ogni messaggio
*/
void listDevices(int responde_to) {
    message_t my_data;
    listDevicesInList(responde_to, disconnected_children, 0);

    my_data = buildListResponse(responde_to, id, 0, 0);
    sprintf(ret.text, CB_CYAN "(0)" CB_WHITE " controller" C_WHITE "\n");
    
    listDevicesInList(responde_to, connected_children, 1);
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
int delDevice(int id) {
    if (id == 0) 
        return 0;

    int pid = getPidById(disconnected_children, id);
    if (pid == -1) pid = getPidById(connected_children, id);
    if (pid == -1) 
        return -1;
    
    message_t request = buildDeleteRequest(pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        return -2;
    } else if (receiveMessage(&response) == -1) {
        return -3;
    } else {
        return 1;
    }
}

/**************************************** LINK ********************************************/
/* Effettua il link di id1 a id2                                                          */
/******************************************************************************************/
int linkDevices(int id1, int id2) {
    if (id1 == 0) 
        return 0;
    
    // Risolvo l'id1 in un PID valido
    int src = getPidById(disconnected_children, id1);
    if (src == -1) src = getPidById(connected_children, id1);
    if (src == -1) 
        return -1;
    

    message_t request, response;
    if (id2 == 0) {
        // Connetto src al controller (id = 0)
        doLink(connected_children, src, base_dir);
        receiveMessage(NULL);  //  Attendo una conferma dal figlio clonato.
        return 1;  //TODO: DESTRO questo punto può essere considerato come una conferma di successo ?
    } else {
        // Risolvo l'id2 in un PID valido
        int dest = getPidById(disconnected_children, id2);
        if (dest == -1) dest = getPidById(connected_children, id2);
        if (dest == -1) 
            return -2;

        // Se il src contiene dest tra i sui figli, sto creando un ciclo.
        request = buildTranslateRequest(src, id2);
        if (sendMessage(&request) == -1)
            return -3;
        else if (receiveMessage(&response) == -1) 
            return -4;
        else if (response.vals[TRANSLATE_VAL_ID] > 0) 
            return -5;
        

        // Invio la richiesta di link a dest con il PID di src
        request = buildLinkRequest(dest, src);
        if (sendMessage(&request) == -1) 
            return -3;
        else if (receiveMessage(&response) == -1) 
            return -4;
        else if (response.vals[LINK_VAL_SUCCESS] == LINK_ERROR_NOT_CONTROL) 
           return -6;
        else if (response.vals[LINK_VAL_SUCCESS] == LINK_ERROR_MAX_CHILD) 
            return -7;
    }
    request = buildDeleteRequest(src); //  Killo il processo src già clonato
    if (sendMessage(&request) == -1)
        return -3;
    else if (receiveMessage(&response) == -1) 
        return -4;
    else 
        return 1;
}



/**************************************** SWITCH ********************************************/
/* Cambia lo stato dell'interruttore "label" del dispositivo "id" al valore "pos"           */
/********************************************************************************************/
int switchDevice(int id, char *label, char *pos) {
    int pid = getPidById(connected_children, id);
    if (pid == -1) {
        if (getPidById(disconnected_children, id) != -1) {
            printf(CB_RED "Error: device with id (%d) not connected to the controller\n" C_WHITE, id);
        } else {
            printf(CB_RED "Error: device with id (%d) not found\n" C_WHITE, id);
        }
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
        printf(CB_RED "Error: invalid label \"%s\"\n" C_WHITE, label);
        return;
    } else if (pos_val == INVALID_VALUE) {
        if (label_val == LABEL_THERM_VALUE) {
            printf(CB_RED "Error: invalid pos value \"%s\" for label \"%s\". It must be a number between -30°C and 15°C \n" C_WHITE, pos, label);
        } else {
            printf(CB_RED "Error: invalid pos value \"%s\" for label \"%s\"" C_WHITE, pos, label);
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
                printf(CB_RED "The label \"%s\" is not supported by the device (%d)\n" C_WHITE, label, id);
            } else {
                printf(CB_GREEN "Switch executed\n" C_WHITE);
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
        printf(CB_RED "Error: device with id (%d) not found\n" C_WHITE, id);
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
            printf(CB_RED "Error: invalid register \"%s\"\n" C_WHITE, label);
            return;
        } else if (pos_val == INVALID_VALUE) {
            printf(CB_RED "Error: invalid value \"%s\" for register \"%s\"\n" C_WHITE, label, val);
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
                    printf(CB_RED "The register \"%s\" is not supported by the device (%d)\n" C_WHITE, label, id);
                } else {
                    if (response.vals[SET_VAL_SUCCESS] == SET_TIMER_STARTED_ON_SUCCESS)
                        printf(CB_GREEN "Set executed (a timer was started)\n" C_WHITE);
                    else
                        printf(CB_GREEN "Set executed\n" C_WHITE);
                }
            }
        }
    }
}

/* Temporany unused
void infoDevice(int id) {
    if (id == 0) {
        printf(CB_CYAN "(0)" CB_WHITE " controller" C_WHITE ", " CB_GREEN "registers:" C_WHITE " num = %d\n", listCount(connected_children));
    } else {
        int pid = getPidById(disconnected_children, id);
        if (pid == -1) pid = getPidById(connected_children, id);
        if (pid == -1) {
            printf(CB_RED "Error: device with id (%d) not found\n" C_WHITE, id);
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
}*/
