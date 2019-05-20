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
int componentInit() {
    // Avvia il processo home comunicandoli la chiave per la coda di messaggi
    char *base_dir;
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
    return 1;
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
                for (i = 0; i < response.vals[LIST_VAL_LEVEL] + response.vals[LIST_VAL_ACTIVE] - 1; i++) printf("    ");  // Indento la stampa in base al livello
                if (response.vals[LIST_VAL_ACTIVE] == 1 || response.vals[LIST_VAL_LEVEL] > 0) printf(C_CYAN " └──" C_WHITE);
                printf(CB_CYAN "(%d)" C_WHITE " %s\n", response.vals[LIST_VAL_ID], response.text);
            }
        } while (response.vals[LIST_VAL_STOP] != 1);
    }
}

/**************************************** ADD ********************************************/
/* Aggiunge un dispositivo al controller in base al tipo specificato                     */
/*****************************************************************************************/




int addDevice(char* device) {
    message_t request = buildAddRequest(home_pid, device);
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


int linkDevices(int id1, int id2) {
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
    /* Faccio il mapping dei comandi da stringhe ad interi */
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
    /* Faccio il mapping da stringhe a valori interi */
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

    if (!isInt(val)){
        return -5;
    }

    message_t request = buildTerminalSetRequest(home_pid, id, label_val, pos_val);
    message_t response;
    if (sendMessage(&request) == -1) 
        perror("Error set request");
    else if (receiveMessage(&response) == -1) 
        perror("Error set response");
    else
        return response.vals[SET_VAL_SUCCESS];
}

/**************************************** INFO ************************************************/
/* Ottiene il sottoalbero a partire dal device "id" con tutte le informazioni dei dispositivi */
/**********************************************************************************************/

//TODO: Steve verificare che il comando sia davvero di una INFO
void infoDevice(int id){
    message_t request = buildTerminalInfoRequest(home_pid, id);
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
