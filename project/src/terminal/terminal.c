#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/constants.h"
#include "../include/utils.h"
#include "./terminal_lib.c"

#define MAX_LEN 256  //  Massima lunghezza stringa parametri
#define MAX_ARGC 10  //  Massimo numero parametri

#define ARGC_HELP 1
#define ARGC_LIST 1
#define ARGC_ADD 2
#define ARGC_DEL 2
#define ARGC_LINK 4
#define ARGC_UNLINK 2
#define ARGC_SWITCH 4
#define ARGC_SET 4
#define ARGC_INFO 2
#define ARGC_QUIT 1

/**
 * Definire MANUAL in compilazione per generare l'esegubile per i comandi manuali
 **/

/*  Lettura e parse parametri */
int getArgs(char *line, int *argc, char **argv);
/*  Print con identazione   */
void printHelp(char *cmd, char *desc);

/*  Metodi implementati nel terminale   */
void terminalInit(char *file);
void terminalDestroy();
#ifndef MANUAL
void listDevices();
int addDevice(char *device);
int unlinkDevices(int id);
#endif
void delDevice(int id);
void linkDevices(int id1, int id2);
void switchDevice(int id, char *label, char *pos);
void setDevice(int id, char *label, char *val);
void infoDevice(int id);
short doExport(FILE *fp, char *file_name, char *file_tmp);
void saveCommand(char *command, int argc, char **argv);
void sighandle_int(int sig);

FILE *fp;           //messa come variabile globale per facilitare la gestione tra MANUAL e TERMINAL, in modo da evitare #ifndef ogni volta che eseguirò il comando saveCommand() {funzione che salva su file il comando appena eseguito }
char file_tmp[32];  //dichiarazione comune a a tutti, ma usato solo da TERMINAL per evitare controlli verbosi sul resto del codice, ma solo nei punti fondamentali

/* Main */
int main(int sargc, char **sargv) {
    snprintf(file_tmp, 32, "tmp_file%d.txt", getpid());

#ifndef MANUAL
    ipcInit(getMq(getpid()));       // Inizializzo componenti comunicazione
    fp = fopen(file_tmp, "w");      //apro il file in cui andrò a salvare i comandi che verranno eseguiti (per implementare comando export)
    signal(SIGINT, sighandle_int);  //gestisco il segnle SIGINT per cancellare il file temporaneo dei comandi anche se si chiude in modo anomalo da tastiera il processo (e non dal normale quit)
#else
    if (sargc <= 1 || !isInt(sargv[1])) {
        printf("Usage: manual <terminal id>\n");
        return 0;
    }
    terminal_pid = atoi(sargv[1]);
    ipcInit(getMq(terminal_pid));  // Inizializzo componenti comunicazione, mi collego al controller passato come parametro
#endif
    terminalInit(sargv[0]);
    short run = 1;       //  Per uscire dal while nel caso si scriva "quit"
    int print_sign = 1;  // Per identificare quando stampare >. Serve per evitare duplicati nel caso venga ricevuto un segnale da una shell manuale
    char line[MAX_LEN];
    int argc = 0;
    char *argv[MAX_ARGC];

#ifndef MANUAL
    printf(CB_BLUE ",------------------------------------------------------------------,\n" C_WHITE);
    printf(CB_BLUE "|" CB_YELLOW "  Fridge       Window         Bulb    Timer       Hub      Alarm  " CB_BLUE "|\n" C_WHITE);
    printf(CB_BLUE "|" CB_WHITE "  ______     ___________      ___      ___       ____        _    " CB_BLUE "|\n" C_WHITE);
    printf(CB_BLUE "|" CB_WHITE "  |4°  |     |    |    |     / _ \\    / L \\     |    =      / \\   " CB_BLUE "|\n" C_WHITE);
    printf(CB_BLUE "|" CB_WHITE "  |   -|     |----|----|     \\ v /    \\___/     |    =     / ! \\  " CB_BLUE "|\n" C_WHITE);
    printf(CB_BLUE "|" CB_WHITE "  |____|     |____|____|      | |               |____=    /_____\\ " CB_BLUE "|\n" C_WHITE);
    printf(CB_BLUE "'------------------------------------------------------------------'\n" C_WHITE);
    printf(CB_WHITE "Domotic System 1.0" C_WHITE ", identifier (use this for manual commands): " CB_WHITE "%d\n" C_WHITE, getpid());
#else
    printf(CB_WHITE "Manual shell" C_WHITE " connected to the controller: " CB_WHITE "%d\n" C_WHITE, terminal_pid);
#endif

    printf("Type \"help\" for more information.\n\n");
    while (run) {
        if (print_sign)
            printf("> ");
        else
            print_sign = 1;

        if (getArgs(line, &argc, argv) == -1) {
            print_sign = 0;
            continue;
        }

        /**************************************** HELP ********************************************/
        if (strcmp(argv[0], "help") == 0) {
            printf("Available commands:\n");
            printHelp("help", "Print this page.");
#ifndef MANUAL  // I comandi LIST e ADD sono supportati solo dalla shell principale e non da quella manuale
            printHelp("list", "List the installed devices.");
            printHelp("add <device>", "Add <device> to the system.");
            printHelp("unlink <id>", "Disconnect a device from the controller.");
#endif
            printHelp("del <id>", "Remove device <id>. If the device is a control device, remove also the linked devices.");
            printHelp("link <id> to <id>", "Link two devices.");
            printHelp("unlink <id>", "Disable component.");
            printHelp("switch <id> <label> <pos>", "Change the value of the switch <label> of the device <id> to <pos>.");
            printHelp("set <id> <register> <value>", "Change the value of the register <register> of the device <id> to <value>.");
            printHelp("info <id>", "Print device <id> info.");
            printHelp("export <file_name>", "Save the current structure to file <file_name>.");
            printHelp("quit", "Close the terminal and kill all processes.");
        }
/**************************************** LIST ********************************************/
#ifndef MANUAL
        else if (strcmp(argv[0], "list") == 0) {
            if (argc != ARGC_LIST) {
                printf(CB_RED "Unknown parameters, usage: list\n" C_WHITE);
            } else {
                listDevices();
            }
        }
        /**************************************** ADD ********************************************/
        else if (strcmp(argv[0], "add") == 0) {
            if (argc != ARGC_ADD) {
                printf(CB_RED "Unknown parameters, usage: add <device>\n" C_WHITE);
            } else {
                int result = 0;
                if (strcmp(argv[1], BULB) == 0)
                    result = addDevice(BULB);
                else if (strcmp(argv[1], FRIDGE) == 0)
                    result = addDevice(FRIDGE);
                else if (strcmp(argv[1], WINDOW) == 0)
                    result = addDevice(WINDOW);
                else if (strcmp(argv[1], ALARM) == 0)
                    result = addDevice(ALARM);
                else if (strcmp(argv[1], HUB) == 0)
                    result = addDevice(HUB);
                else if (strcmp(argv[1], TIMER) == 0)
                    result = addDevice(TIMER);
                else
                    printf(CB_RED "Unknown device, supported devices: bulb, fridge, window, hub, timer, alarm\n" C_WHITE);

                if (result == -1)
                    perror(CB_RED "Error while adding device" C_WHITE);
                else if (result != 0) {  //  Se ho aggiunto un device supportato
                    printf(CB_GREEN "Device added with id %d" C_WHITE "\nNow it's disconnected from the system. To connect the device run " CB_WHITE "link %d to 0\n" C_WHITE, result, result);
                    saveCommand(line, argc, argv);
                }
            }
        }
        /**************************************** UNLINK ********************************************/
        else if (strcmp(argv[0], "unlink") == 0) {
            if (argc != ARGC_UNLINK != 0) {
                printf(CB_RED "Unknown parameters, usage: unlink <id>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                int res = unlinkDevices(atoi(argv[1]));
                if (res > 0)
                    printf(CB_GREEN "Device %d disconnected from the controller\n" C_WHITE, atoi(argv[1]));
                saveCommand(line, argc, argv);
            }
        }
        /**************************************** EXPORT ********************************************/
        else if (strcmp(argv[0], "export") == 0) {
            fclose(fp);
            if (doExport(fp, argv[1], file_tmp) > 0)
                printf(CB_GREEN "%s saved\n" C_WHITE, argv[1]);
            else
                printf(CB_RED "Error while saving %s\n" C_WHITE, argv[1]);
            fp = fopen(file_tmp, "a");  //riapro in append per non sovrascrivere i comandi di questa stessa sessione
        }
#endif
        /**************************************** DEL ********************************************/
        else if (strcmp(argv[0], "del") == 0) {
            if (argc != ARGC_DEL) {
                printf(CB_RED "Unknown parameters, usage: del <id>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                delDevice(atoi(argv[1]));
                saveCommand(line, argc, argv);
            }
        }
        /**************************************** LINK ********************************************/
        else if (strcmp(argv[0], "link") == 0) {
            if (argc != ARGC_LINK || strcmp(argv[2], "to") != 0) {
                printf(CB_RED "Unknown parameters, usage: link <id> to <id>\n" C_WHITE);
            } else if (!isInt(argv[1]) || !isInt(argv[3]) || atoi(argv[1]) < 0 || atoi(argv[3]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else if (strcmp(argv[1], argv[3]) == 0) {
                printf(CB_RED "Error: cannot link a device to itself\n" C_WHITE);
            } else {
                linkDevices(atoi(argv[1]), atoi(argv[3]));
                saveCommand(line, argc, argv);
            }
        }
        /**************************************** SWITCH ********************************************/
        else if (strcmp(argv[0], "switch") == 0) {
            if (argc != ARGC_SWITCH) {
                printf(CB_RED "Unknown parameters, usage: switch <id> <label> <pos>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                switchDevice(atoi(argv[1]), argv[2], argv[3]);
                saveCommand(line, argc, argv);
            }
        }
        /**************************************** SET ********************************************/
        else if (strcmp(argv[0], "set") == 0) {
            if (argc != ARGC_SET) {
                printf(CB_RED "Unknown parameters, usage: set <id> <register> <value>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                setDevice(atoi(argv[1]), argv[2], argv[3]);
                saveCommand(line, argc, argv);
            }
        }
        /**************************************** INFO ********************************************/
        else if (strcmp(argv[0], "info") == 0) {
            if (argc != ARGC_INFO) {
                printf(CB_RED "Unknown parameters, usage: info <id>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                infoDevice(atoi(argv[1]));
            }
        }
        /**************************************** QUIT ********************************************/
        else if (strcmp(argv[0], "quit") == 0) {
            run = 0;  //termino l'esecuzione
        } else if (argc > 0) {
            printf(CB_RED "Unknown command, type \"help\" to list all the supported commands\n" C_WHITE);
        }
    }
    fclose(fp);

    if (remove(file_tmp) < 0)
        perror("Error while deleting tmp command file");

    terminalDestroy();
    return 0;
}

int getArgs(char *line, int *argc, char **argv) {
    // Lettura stringa
    if (fgets(line, MAX_LEN, stdin) == NULL) return -1;  // Per evitare la ripetizione di comandi in caso venga ricevuto un segnale
    line[strcspn(line, "\n")] = '\0';                    //  Rimuovo eventuali \n dalla fine della stringa per evitare problemi nel parse
    // Parse argomenti
    int pos = 0;
    char *arg = strtok(line, " ");  //  Parse primo parametro
    while (arg != NULL && pos < MAX_ARGC) {
        argv[pos++] = arg;
        arg = strtok(NULL, " ");  //  Parse parametro successivo
    }
    *argc = pos;
    return 0;
}

void printHelp(char *cmd, char *desc) {
    printf("    %-28s%s\n", cmd, desc);
}

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

/*
  Gestore del segnale per eliminare i file temporanei. Solo per TERMINAL
*/
#ifndef MANUAL
void sighandle_int(int sig) {
    if (remove(file_tmp) < 0)
        perror("Error while deleting tmp command file");
    terminalDestroy();
    exit(0);
}
#endif
