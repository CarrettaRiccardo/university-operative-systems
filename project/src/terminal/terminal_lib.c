#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../include/ipc.h"
#include "../include/list.h"

/**
 * Implementa i metodi definiti in terminal.c
*/

#ifndef MANUAL
int next_id;
list_t children;  // Lista che contiene i pid del controller e di tutti i componenti aggiunti ma non attivi

char *base_dir;
FILE *fp;           // Variabile globale per facilitare la gestione tra MANUAL e TERMINAL, in modo da evitare #ifndef ogni volta che eseguirò il comando saveCommand() {funzione che salva su file il comando appena eseguito }
char file_tmp[32];  // Dichiarazione comune a a tutti, ma usato solo da TERMINAL per evitare controlli verbosi sul resto del codice, ma solo nei punti fondamentali

/* Gestore del segnale per eliminare i file temporanei. Usato solo per TERMINAL */
void sighandleInt(int sig) {
    if (remove(file_tmp) < 0)
        perror("Error while deleting tmp command file");
    printf(CB_WHITE "System disconnected\n" C_WHITE);
    exit(0);
}

/* Handler segnale eliminazione figli */
void sigchldHandler(int signum) {
    int pid;
    do {
        pid = waitpid(-1, NULL, WNOHANG);
        listRemove(children, &pid);
    } while (pid != -1 && pid != 0);
}

/* Handler richiesta traduzione figlio */
void getPidByIdSignalHandler(int sig, siginfo_t *siginfo, void *context) {
    int to_solve_id = siginfo->si_value.sival_int;
    int pid = getPidById(children, to_solve_id);
    if (sendGetPidByIdSignal(siginfo->si_pid, pid) < 0) {
        perror("Error: cannot send response to manual shell");
    }
}
/* Ritorna il pid del device (id) */
int solveId(id) {
    return getPidById(children, id);
}

/* Ritorna 0 se il controller è disabilitato, 1 altrimenti */
int isControllerEnabled(int controller_pid) {
    message_t request = buildCloneRequest(controller_pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error checking controller enabled request\n");
    } else if (receiveMessage(&response) == -1) {
        perror("Error checking controller enabled response\n");
    } else {
        return response.vals[3];  // Valore di stato del controller
    }
    return 0;
}

/* Funzione di supporto per ottenre il PID del controller, che è sempre l'ultimo valore della lista dei figli */
int getControllerPid() {
    void *controller_pid = listLast(children);
    if (controller_pid == NULL) {
        printf(CB_RED "Error: controller not found, aborting...\n" C_WHITE);
        exit(1);
    }
    return *(int *)controller_pid;
}
#else
int terminal_pid;  // PID del terminale collegato
int solved_pid;    // Ultimo PID risolto.

/* Chiede al terminale con id "terminal_id" il pid collegato al device con l'id passato come parametro */
int solveId(int id) {
    if (sendGetPidByIdSignal(terminal_pid, id) < 0) {
        printf(CB_RED "Error: cannot contact the terminal. Check the terminal id and retry.\nClosing...\n" C_WHITE);
        exit(0);
        return -1;
    } else {
        pause();  // Attendo la ricezione del segnale di risposta da parte del controller prima di continuare
        return solved_pid;
    }
}

/* Handler risposta getPidById dal controller */
static void getPidByIdSignalHandler(int sig, siginfo_t *siginfo, void *context) {
    solved_pid = siginfo->si_value.sival_int;  // Imposto il valore del pid risolto dal controller
}
#endif

/**************************************** INIT ********************************************/
/* Inizializzazione valori terminal                                                     */
/******************************************************************************************/
/* file: percorso dell' eseguibile corrente, usato per estrarre la cartella di origine da cui si è eseguito il programma */
void terminalInit(char *file) {
    // Registrazione handler per risposta getPidById dal terminal. Se MANUAL è settato registra come handler il metodo apposito
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_sigaction = getPidByIdSignalHandler;
    sig.sa_flags = SA_SIGINFO;
    if (sigaction(SIGUSR1, &sig, NULL) < 0) {
        perror("Error: cannot register SIGUSR1 handler");
    }

#ifndef MANUAL
    // Registrazione handler per signal terminazione figli
    signal(SIGCHLD, sigchldHandler);
    signal(SIGINT, sighandleInt);  //gestisco il segnle SIGINT per cancellare il file temporaneo dei comandi anche se si chiude in modo anomalo da tastiera il processo (e non dal normale quit)
    children = listIntInit();
    next_id = 0;  //il primo componetne avrà id = 0 (ovvero il controller)

    //  Uso il percorso relativo al workspace, preso da argv[0] per trovare gli altri eseguibili per i device.
    //  Rimuovo il nome del file dal percorso
    base_dir = extractBaseDir(file);
    if (addDevice(CONTROLLER)) {
        printf(CB_RED "Error: cannot create the controller, aborting...\n" C_WHITE);
        exit(1);
    }

    snprintf(file_tmp, 32, "%stmp_file%d.txt", base_dir, getpid());  // genero la stringa che rappresenta il file temporaneo dei comandi nel path di ./bin
    fp = fopen(file_tmp, "w");                                       // apro il file in cui andrò a salvare i comandi che verranno eseguiti (per implementare comando export)
#else
    // Nella shell manuale non devo aggiungere il controller come figlio o settare l'handler per i segnali di kill dei figli
    solved_pid = -1;
#endif
}

/*  Dealloca il terminal  */
void terminalDestroy() {
#ifndef MANUAL
    // Eliminazione di tutti i figli per evitare processi zombie
    node_t *p = children->head;
    while (p != NULL) {
        message_t request = buildDeleteRequest(*(int *)p->value);
        message_t response;
        if (sendMessage(&request) == -1)
            perror("Error deleting device request");
        else if (receiveMessage(&response) == -1)
            perror("Error deleting device response");
        p = p->next;
    }
    listDestroy(children);
    free(base_dir);

    fclose(fp);  // Chiudo il file temporaneo usato per salvare la cronologia dei messaggi e lo elimino
    if (remove(file_tmp) < 0) perror("Error while deleting tmp command file");
    closeMq();  // Chiudo la message queue
#endif
}

#ifndef MANUAL  // I seguenti comandi non sono supportati dalla shell manuale
/**************************************** LIST ********************************************/
/* Stampa l'albero dei dispositivi con l'id e lo stato                                     */
/******************************************************************************************/
void listDevices() {
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
                } else if (response.type == LIST_MSG_TYPE) {  //controllo che non sia stato un mix di messaggi, in tal caso ignoro il messaggio
                    int i;
                    for (i = 0; i < response.vals[INFO_VAL_LEVEL] - 1; i++) printf("    ");  // Stampa x \t, dove x = lv (profondità componente, per indentazione)
                    if (response.vals[INFO_VAL_LEVEL] > 0) printf(C_CYAN " └──" C_WHITE);
                    printf(CB_CYAN "(%d)" C_WHITE " %s\n", response.vals[INFO_VAL_ID], response.text);
                }
            } while (response.vals[INFO_VAL_STOP] != 1);
        }
        p = p->next;
    }
}

/**************************************** ADD ********************************************/
/* Aggiunge un dispositivo al terminale in base al tipo specificato                      */
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
        listPushFront(children, &pid, sizeof(int));
        return next_id - 1;
    }
}

/**************************************** UNLINK ********************************************/
/* Disabilita un componente, rendendolo non più interagibile dal controller                 */
/* Operazione ammessa solamente da terminal e non comando manuale (il quale può al          */
/* più fare un DELETE)                                                                      */
/********************************************************************************************/
int unlinkDevice(int id) {
    int to_pid = solveId(id);
    if (to_pid == -1) {
        printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id);
        return -1;
    }

    if (listContains(children, &to_pid)) {  // è già presente nella lista dei figli del terminal, quindi è già disabilitato
        printf(CB_YELLOW "Device already disconnected\n" C_WHITE);
        return -1;
    }

    int res = doLink(children, to_pid, base_dir, 1);
    message_t ack;  // Aspetto la conferma di avvenuta link dal dispositivo
    receiveMessage(&ack);
    if (res < 0) return -1;

    //  Killo il processo disabilitato
    message_t request, response;
    request = buildDeleteRequest(to_pid);
    if (sendMessage(&request) == -1) {
        perror("Error deleting device request");
        return -1;
    } else if (receiveMessage(&response) == -1) {
        perror("Error deleting device response");
        return -1;
    }
    printf(CB_GREEN "Device %d disconnected\n" C_WHITE, id);
    return 0;
}
#endif

/**************************************** DEL ********************************************/
/* Elimina un dispositivo in base all'id specificato                                     */
/*****************************************************************************************/
int delDevice(int id) {
    if (id == 0) {
        printf(CB_RED "Error: the controller cannot be deleted\n" C_WHITE);
        return -1;
    }

    int pid = solveId(id);
    if (pid == -1) {
        printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id);
        return -1;
    }
    message_t request = buildDeleteRequest(pid);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error deleting device request");
        return -1;
    } else if (receiveMessage(&response) == -1) {
        perror("Error deleting device response");
        return -1;
    }
    printf(CB_GREEN "Device %d deleted\n" C_WHITE, id);
    return 0;
}

/**************************************** LINK ********************************************/
/* Effettua il link di id1 a id2                                                          */
/******************************************************************************************/
int linkDevices(int id1, int id2) {
    if (id1 == 0) {
        printf(CB_RED "Error: cannot connect the controller to other devices\n" C_WHITE);
        return -1;
    }

    // Risolvo l'id1 in un PID valido
    int src = solveId(id1);
    if (src == -1) {
        printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id1);
        return -1;
    }

    message_t request, response;
    // Risolvo l'id2 in un PID valido
    int dest = solveId(id2);
    if (dest == -1) {
        printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id2);
        return -1;
    }

    // Se il src contiene dest tra i sui figli, sto creando un ciclo.
    if (getPidByIdSingle(src, id2) > 0) {
        printf(CB_RED "Error: cycle identified, %d is a child of %d\n" C_WHITE, id2, id1);
        return -1;
    }

    // Invio la richiesta di link a dest con il PID di src
    request = buildLinkRequest(dest, src);
    if (sendMessage(&request) == -1) {
        perror("Error linking devices request");
        return -1;
    } else if (receiveMessage(&response) == -1) {
        perror("Error linking devices response");
        return -1;
    } else if (response.vals[LINK_VAL_SUCCESS] == LINK_ERROR_NOT_CONTROL) {
        printf(CB_RED "Error: the device with id %d is not a control device\n" C_WHITE, id2);
        return -1;
    } else if (response.vals[LINK_VAL_SUCCESS] == LINK_ERROR_MAX_CHILD) {
        printf(CB_RED "Error: the device with id %d already has a child\n" C_WHITE, id2);
        return -1;
    }
    //  Killo il processo src già clonato
    request = buildDeleteRequest(src);
    if (sendMessage(&request) == -1) {
        perror("Error deleting device request");
        return -1;
    } else if (receiveMessage(&response) == -1) {
        perror("Error deleting device response");
        return -1;
    }
    printf(CB_GREEN "Device %d linked to %d\n" C_WHITE, id1, id2);
    return 0;
}

/**************************************** SWITCH ********************************************/
/* Cambia lo stato dell'interruttore "label" del dispositivo "id" al valore "pos"           */
/* Uno switch può essere fatto solo su dispostivi attivi                                    */
/********************************************************************************************/

int switchDevice(int id, char *label, char *pos) {
#ifndef MANUAL
    int controller_pid = getControllerPid();
    if (id != 0 && isControllerEnabled(controller_pid) == 0) {
        printf(CB_RED "Error: the controller is disabled. Run " CB_WHITE "switch 0 general on" CB_RED " to enable it\n" C_WHITE);
        return -1;
    }

    int pid = getPidByIdSingle(controller_pid, id);
    if (pid == -1) {
        if (getPidById(children, id) != -1) {
            printf(CB_RED "Error: device with id %d not connected to the controller\n" C_WHITE, id);
        } else {
            printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id);
        }
        return -1;
    }

#else
    int pid = solveId(id);
    if (pid == -1) {
        printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id);
        return;
    }
#endif
    int label_val = INVALID_VALUE;
    int pos_val = INVALID_VALUE;  // 0 = spento, 1 = acceso; x = valore termostato (°C)
    // Map delle label (char*) in valori (int) per poterli inserire in un messaggio
    if (strcmp(label, LABEL_BULB_LIGHT) == 0) {
        label_val = LABEL_BULB_LIGHT_VALUE;
    } else if (strcmp(label, LABEL_WINDOW_OPEN) == 0) {
        label_val = LABEL_WINDOW_OPEN_VALUE;
    } else if (strcmp(label, LABEL_WINDOW_CLOSE) == 0) {
        label_val = LABEL_WINDOW_CLOSE_VALUE;
    } else if (strcmp(label, LABEL_FRIDGE_DOOR) == 0) {
        label_val = LABEL_FRIDGE_DOOR_VALUE;
    } else if (strcmp(label, LABEL_FRIDGE_THERM) == 0) {
        label_val = LABEL_FRIDGE_THERM_VALUE;
    } else if (strcmp(label, LABEL_ALARM_ENABLE) == 0) {
        label_val = LABEL_ALARM_ENABLE_VALUE;
    } else if (strcmp(label, LABEL_ALL) == 0) {
        label_val = LABEL_ALL_VALUE;
    } else if (strcmp(label, LABEL_GENERAL) == 0) {
        label_val = LABEL_GENERAL_VALUE;
    }

    // Map valore pos (char*) in valori (int) per poterli inserire in un messaggio
    if (label_val == LABEL_BULB_LIGHT_VALUE || label_val == LABEL_WINDOW_OPEN_VALUE || label_val == LABEL_WINDOW_CLOSE_VALUE || label_val == LABEL_FRIDGE_DOOR_VALUE || label_val == LABEL_ALARM_ENABLE_VALUE || label_val == LABEL_ALL_VALUE || label_val == LABEL_GENERAL_VALUE) {
        // Se è un interrutore on/off
        if (strcmp(pos, SWITCH_POS_OFF_LABEL) == 0) {
            pos_val = SWITCH_POS_OFF_LABEL_VALUE;  // 0 = spento/chiuso
        } else if (strcmp(pos, SWITCH_POS_ON_LABEL) == 0) {
            pos_val = SWITCH_POS_ON_LABEL_VALUE;  // 1 = acceso/aperto
        }
    } else if (label_val == LABEL_FRIDGE_THERM_VALUE) {
        if (isInt(pos) && atoi(pos) >= -20 && atoi(pos) <= 15) {
            pos_val = atoi(pos);
        }
    }

    // Se i parametri creano dei valori validi
    if (label_val == INVALID_VALUE) {
        printf(CB_RED "Error: invalid label \"%s\"\n" C_WHITE, label);
        return -1;
    } else if (pos_val == INVALID_VALUE) {
        if (label_val == LABEL_FRIDGE_THERM_VALUE) {
            printf(CB_RED "Error: invalid pos value %s°C for label \"%s\". It must be a number between -20°C and 15°C \n" C_WHITE, pos, label);
        } else {
            printf(CB_RED "Error: invalid pos value \"%s\" for label \"%s\"\n" C_WHITE, pos, label);
        }
        return -1;
    } else {
        if (label_val == LABEL_GENERAL_VALUE && id != 0) {  // L'interrruttore GENERAL è utilizzabile solo nel controller
            printf(CB_RED "Error: the label \"%s\" is not supported by the device %d\n" C_WHITE, label, id);
            return -1;
        }
        message_t request = buildSwitchRequest(pid, label_val, pos_val);
        message_t response;
        if (sendMessage(&request) == -1) {
            perror("Error switch request");
            return -1;
        } else if (receiveMessage(&response) == -1) {
            perror("Error switch response");
            return -1;
        } else if (response.vals[SWITCH_VAL_SUCCESS] == SWITCH_ERROR_INVALID_VALUE) {
            printf(CB_RED "Error: the label \"%s\" is not supported by the device %d\n" C_WHITE, label, id);
            return -1;
        }
        printf(CB_GREEN "Switch executed\n" C_WHITE);
        return 0;
    }
}

/**************************************** SET ***********************************************/
/* Cambia lo stato del "register" del dispositivo "id" al valore "val"                      */
/* Il SET può essere fatto solo su dispositivi attivi                                       */
/********************************************************************************************/
int setDevice(int id, char *label, char *val) {
#ifndef MANUAL
    int controller_pid = getControllerPid();
    if (id != 0 && isControllerEnabled(controller_pid) == 0) {
        printf(CB_RED "Error: the controller is disabled. Run " CB_WHITE "switch 0 general on" CB_RED " to enable it\n" C_WHITE);
        return -1;
    }

    int pid = getPidByIdSingle(controller_pid, id);
    if (pid == -1) {
        if (getPidById(children, id) != -1) {
            printf(CB_RED "Error: device with id %d not connected to the controller\n" C_WHITE, id);
        } else {
            printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id);
        }
        return -1;
    }
#else
    int pid = solveId(id);
    if (pid == -1) {
        printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id);
        return -1;
    }
#endif
    int label_val = INVALID_VALUE;  // 0 = interruttore (generico), 1 = termostato
    int pos_val = INVALID_VALUE;    // 0 = spento, 1 = acceso; x = valore termostato (°C)
    // Map delle label (char*) in valori (int) per poterli inserire in un messaggio
    if (strcmp(label, REGISTER_DELAY) == 0) {
        label_val = REGISTER_DELAY_VALUE;  // 1 = delay (fridge)
    } else if (strcmp(label, REGISTER_BEGIN) == 0) {
        label_val = REGISTER_BEGIN_VALUE;  // 2 = begin (timer)
    } else if (strcmp(label, REGISTER_END) == 0) {
        label_val = REGISTER_END_VALUE;  // 4 = end (timer)
    } else if (strcmp(label, REGISTER_PERC) == 0) {
        label_val = REGISTER_PERC_VALUE;  // 8 = perc (fridge)
    } else if (strcmp(label, REGISTER_PROB) == 0) {
        label_val = REGISTER_PROB_VALUE;  // 16 = prob (alarm)
    }

    // valore del delay, di inizio o fine timer
    if (label_val == REGISTER_DELAY_VALUE && isInt(val) && atoi(val) > 0) {  // E' un valore valido solo se è un numero (i register sono delay, begin o end)
        pos_val = atoi(val);
    } else if (label_val == REGISTER_BEGIN_VALUE || label_val == REGISTER_END_VALUE) {  // se è begin/end, il numero inserito indica quanti seconda da ORA
        int hr = 0;
        int min = 0;
        int sec = 0;
        if (sscanf(val, "%d:%d:%d", &hr, &min, &sec) == 0)// 0 = nessun parametro è stato salvato (quindi hr passato non era un intero)
            pos_val = INVALID_VALUE;
        else {
            if (hr >= 0 && hr < 24 && min >= 0 && min < 60 && sec >= 0 && sec < 60) {
                time_t time_now = time(NULL);
                struct tm now = *localtime(&time_now);
                hr -= now.tm_hour;
                min -= now.tm_min;
                sec -= now.tm_sec;
                pos_val = time(NULL) + (hr * 3600) + (min * 60) + sec;
            }
        }
    } else if ((label_val == REGISTER_PERC_VALUE || label_val == REGISTER_PROB_VALUE) && isInt(val) && atoi(val) >= 0 && atoi(val) <= 100) {
        pos_val = atoi(val);
    }

    // Se i parametri creano dei valori validi
    if (label_val == INVALID_VALUE) {
        printf(CB_RED "Error: invalid register \"%s\"\n" C_WHITE, label);
        return -1;
    } else if (pos_val == INVALID_VALUE) {
        if (label_val == REGISTER_PERC_VALUE) {
            printf(CB_RED "Error: invalid value %s%% for register \"%s\". It must be a number between 0 and 100\n" C_WHITE, val, label);
        } else {
            printf(CB_RED "Error: invalid value \"%s\" for register \"%s\"\n" C_WHITE, val, label);
        }
        return -1;
    }
    message_t request = buildSetRequest(pid, label_val, pos_val);
    message_t response;
    if (sendMessage(&request) == -1) {
        perror("Error set request");
    } else if (receiveMessage(&response) == -1) {
        perror("Error set response");
    } else if (response.vals[SET_VAL_SUCCESS] == SET_ERROR_INVALID_VALUE) {
        printf(CB_RED "Error: the register \"%s\" is not supported by the device %d\n" C_WHITE, label, id);
    }

    if (response.vals[SET_VAL_SUCCESS] == SET_TIMER_STARTED_SUCCESS)
        printf(CB_GREEN "Set executed (timer started)\n" C_WHITE);
    else
        printf(CB_GREEN "Set executed\n" C_WHITE);
    return 0;
}

/**************************************** INFO ************************************************/
/* Ottiene il sottoalbero a partire dal device "id" con tutte le informazioni dei dispositivi */
/**********************************************************************************************/
void infoDevice(int id) {
    int pid = solveId(id);
    if (pid == -1) {
        printf(CB_RED "Error: device with id %d not found\n" C_WHITE, id);
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

/**************************************** EXPORT ************************************************/
/* Esporta la struttura del sistema corrente per ripristinarlo successivamente                  */
/************************************************************************************************/
void saveCommand(char *command, int argc, char **argv) {
#ifndef MANUAL
    if (fp != NULL) {
        int i;
        for (i = 0; i < argc; i++) {
            fprintf(fp, "%s ", argv[i]);  //salvo su un file temporaneo tutti i comandi eseguiti, così per fare il comando export copio semplicemente tutti i comandi eseguti in sequenza
        }
        fprintf(fp, "\n");
    }
#endif
}

#ifndef MANUAL
void doExport(char *file_name) {
    FILE *new, *old;
    char c;
    new = fopen(file_name, "w");
    old = fopen(file_tmp, "r");
    if (new == NULL || old == NULL) {
        printf(CB_RED "Error while exporting configuration to \"%s\": %s\n" C_WHITE, file_name, strerror(errno));
        return;
    }

    // copio il contenuto del primo file nel secondo
    c = fgetc(old);
    while (c != EOF) {
        fputc(c, new);
        c = fgetc(old);
    }
    fclose(new);
    fclose(old);
    printf(CB_GREEN "Current configuration saved in \"%s\"\n" C_WHITE, file_name);
}
#endif
