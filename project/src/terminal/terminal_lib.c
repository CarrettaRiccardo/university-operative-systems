#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ipc.h"
#include "../include/list.h"



int home_pid;  //numero processo componente home

/* Handler segnale eliminazione figli */
void sigchldHandler(int signum) {
    int pid;
    do {
        pid = waitpid(-1, NULL, WNOHANG);
        printf("Sono terminal, processo %d morto", pid); //TODO: remove this, just for debug
    } while (pid != -1 && pid != 0);
}


/**************************************** INIT ********************************************/
/* Inizializzazione valori terminale                                                     */
/******************************************************************************************/
void componentInit() {
    // Avvia il processo home comunicandoli la chiave per la coda di messaggi
    int pid = fork();
    if (pid == 0) {   /*  Processo figlio */
        char mqid_str[20], id_str[20];
        strcat(base_dir, HOME);             //  Genero il path dell'eseguibile
        snprintf(mqid_str, 20, "%d", mqid);   //  Converto mqid in stringa
        char *args[] = {base_dir, mqid_str, NULL};
        return execvp(args[0], args);
    }
    else if (pid == -1)
        return -1;
    else
        home_pid = pid;
}



/**************************************** LIST ********************************************/
/* Stampa l'albero dei dispositivi con l'id e lo stato                                     */
/******************************************************************************************/
void listDevices() {
    message_t request = buildListRequest(home_pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error list request terminal");
    } else {
        do {
            if (receiveMessage(&response) == -1) {
                perror("Error list response terminal");
            } else {
                int i;
                for (i = 0; i < response.vals[LIST_VAL_LEVEL] + show_tree - 1; i++) printf("    ");  // Indento la stampa in base al livello
                if (show_tree == 1 || response.vals[LIST_VAL_LEVEL] > 0) printf(C_CYAN " └──" C_WHITE);
                printf(CB_CYAN "(%d)" C_WHITE " %s\n", response.vals[LIST_VAL_ID], response.text);
            }
        } while (response.vals[LIST_VAL_STOP] != 1);
    }
}

/**************************************** ADD ********************************************/
/* Aggiunge un dispositivo al controller in base al tipo specificato                     */
/*****************************************************************************************/




int addDevice(char* device) {
    message_t request = buildAddRequest(home_pid);
    message_t response;
    if (sendMessage(&request) == -1) 
        perror("Error list request terminal");
    else if (receiveMessage(&response) == -1)
        return response.vals[ADD_VAL_RESPONSE];
}

/**************************************** DEL ********************************************/
/* Elimina un dispositivo in base all'id specificato                                     */
/*****************************************************************************************/
int delDevice(int id) {
    message_t request = buildTerminalDeleteRequest(home_pid, id);
    message_t response;
    if (sendMessage(&request) == -1) 
        perror("Error deleting device request terminal");
    else if (receiveMessage(&response) == -1) 
        perror("Error deleting reading response terminal");
    else
        return response.vals[DELETE_VAL_RESPONSE];
}

/**************************************** LINK ********************************************/
/* Invia a home di fare il link di id1 a id2                                              */
/******************************************************************************************/


void linkDevices(int id1, int id2) {
    message_t request, response;
    request = buildTerminalLinkRequest(home_pid,id1,id2);
    if (sendMessage(&request) == -1) {
        perror("Error link request terminal");
    } else if (receiveMessage(&response) == -1) {
        perror("Error link reading response terminal");
    } else {
        return response.vals[LINK_VAL_SUCCESS];
    }
}



/**************************************** SWITCH ********************************************/
/* Invia ad home di cambiare lo stato dell'interruttore "label" del dispositivo             */
/* "id" al valore "pos"                                                                     */
/********************************************************************************************/
int switchDevice(int id, char *label, char *pos) {
    message_t request = buildTerminalSwitchRequest(home_pid, id, label_val, pos_val);
    message_t response;
    if (sendMessage(&request) == -1) 
        perror("Error switch request terminal");
    else if (receiveMessage(&response) == -1) 
        perror("Error switch reading response terminal");
    else 
        return response.vals[SWITCH_VAL_SUCCESS];
}

/**************************************** SET ***********************************************/
/* Cambia lo stato del "register" del dispositivo "id" al valore "val"                      */
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

/**************************************** INFO ************************************************/
/* Ottiene il sottoalbero a partire dal device "id" con tutte le informazioni dei dispositivi */
/**********************************************************************************************/

//TODO: Steve verificare che il comando sia davvero di una INFO
void infoDevice(int id){
    message_t request = buildTerminalInfoRequest(pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error info sending message terminal");
    } else {
        do {
            if (receiveMessage(&response) == -1) {
                perror("Error info reading response terminal");
            } else {
                int i;
                for (i = 0; i < response.vals[INFO_VAL_LEVEL]; i++) {
                    if (i == response.vals[INFO_VAL_LEVEL] - 1)
                        printf(C_CYAN " └──" C_WHITE);
                    else
                        printf("    ");  // Idento la stampa
                }
                printf(CB_CYAN "(%d)" C_WHITE " %s\n", response.vals[INFO_VAL_ID], response.text);
            }
        } while (response.vals[INFO_VAL_STOP] != 1);  //fino a quando ricevo l'ultimo messaggio (con flag stop) continuo a leggere
    }
}
