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
#define ARGC_EXPORT 2
#define ARGC_SWITCH 4
#define ARGC_SET 4
#define ARGC_INFO 2
#define ARGC_QUIT 1

/**
 * Definire MANUAL in compilazione per generare l'esegubile per i comandi manuali
 **/

/*  Lettura e parse parametri */
int getArgs(char *line, int *argc, char **argv, FILE *from);
/*  Print con identazione   */
void printHelp(char *cmd, char *desc);

/*  Metodi implementati nel terminale   */
void terminalInit(char *file);
void terminalDestroy();
#ifndef MANUAL
void listDevices();
int addDevice(char *device);
int unlinkDevice(int id);
#endif
int delDevice(int id);
int linkDevices(int id1, int id2);
int switchDevice(int id, char *label, char *pos);
int setDevice(int id, char *label, char *val);
void infoDevice(int id);
void doExport(char *file_name);
void saveCommand(char *line, int argc, char **argv);

/* Main */
int main(int sargc, char **sargv) {
    FILE *import = NULL;
#ifndef MANUAL
    ipcInit(getMq(getpid()));  // Inizializzo componenti comunicazione
    if (sargc == 2) {          // Lettura file da paramtro per importazione configurazione
        import = fopen(sargv[1], "r");
        if (import == NULL) {
            printf(CB_RED "Error while importing configuration from \"%s\": %s.\nSkipping import...\n\n" C_WHITE, sargv[1], strerror(errno));
        }
    }
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
    printf(CB_BLUE "|" CB_WHITE "  |" CB_CYAN "4°" CB_WHITE "  |     |    |    |     /" CB_YELLOW " _" CB_WHITE " \\    / L \\     |    =      / \\   " CB_BLUE "|\n" C_WHITE);
    printf(CB_BLUE "|" CB_WHITE "  |   -|     |----|----|     \\ v /    \\___/     |    =     / " CB_RED "!" CB_WHITE " \\  " CB_BLUE "|\n" C_WHITE);
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

        if (getArgs(line, &argc, argv, (import == NULL) ? stdin : import) == -1) {
            print_sign = 0;
            if (import != NULL) {
                fclose(import);
                import = NULL;
            }
            continue;
        }

        /**************************************** HELP ********************************************/
        if (strcmp(argv[0], "help") == 0) {
#ifndef MANUAL
            printf(CB_CYAN "To import an existing configuration, run \"terminal <file_name>\"\n\n" C_WHITE);
#endif
            printf("Available commands:\n");
            printHelp("help", "Print this page.");
#ifndef MANUAL  // I comandi LIST e ADD sono supportati solo dalla shell principale e non da quella manuale
            printHelp("list", "List the installed devices.");
            printHelp("add <device>", "Add <device> to the system.");
            printHelp("unlink <id>", "Disconnect a device from its parent.");
            printHelp("export <file_name>", "Save the current configuration to file <file_name>.");
#endif
            printHelp("del <id>", "Remove device <id>. If the device is a control device, remove also the linked devices.");
            printHelp("link <id> to <id>", "Link two devices.");
            printHelp("switch <id> <label> <pos>", "Change the value of the switch <label> of the device <id> to <pos>.");
            printHelp("set <id> <register> <value>", "Change the value of the register <register> of the device <id> to <value>.");
            printHelp("info <id>", "Print device <id> info.");
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
                else if (result != 0) {  //  Se ho aggiunto un device supportato mi ritorna l'id del device appena aggiunto
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
                if (unlinkDevice(atoi(argv[1])) != -1) {
                    saveCommand(line, argc, argv);
                }
            }
        }
        /**************************************** EXPORT ********************************************/
        else if (strcmp(argv[0], "export") == 0) {
            if (argc != ARGC_EXPORT != 0) {
                printf(CB_RED "Unknown parameters, usage: export <file_name>\n" C_WHITE);
            } else {
                fclose(fp);  //chiudo per rendere effettivi i cambiamenti al file
                doExport(argv[1]);
                fp = fopen(file_tmp, "a");  //riapro in append per non sovrascrivere i comandi di questa stessa sessione
            }
        }
#endif
        /**************************************** DEL ********************************************/
        else if (strcmp(argv[0], "del") == 0) {
            if (argc != ARGC_DEL) {
                printf(CB_RED "Unknown parameters, usage: del <id>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                if (delDevice(atoi(argv[1])) != -1) {
                    saveCommand(line, argc, argv);
                }
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
                if (linkDevices(atoi(argv[1]), atoi(argv[3])) != -1) {
                    saveCommand(line, argc, argv);
                }
            }
        }
        /**************************************** SWITCH ********************************************/
        else if (strcmp(argv[0], "switch") == 0) {
            if (argc != ARGC_SWITCH) {
                printf(CB_RED "Unknown parameters, usage: switch <id> <label> <pos>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                if (switchDevice(atoi(argv[1]), argv[2], argv[3]) != -1) {
                    saveCommand(line, argc, argv);
                }
            }
        }
        /**************************************** SET ********************************************/
        else if (strcmp(argv[0], "set") == 0) {
            if (argc != ARGC_SET) {
                printf(CB_RED "Unknown parameters, usage: set <id> <register> <value>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) < 0) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                if (setDevice(atoi(argv[1]), argv[2], argv[3]) != -1) {
                    saveCommand(line, argc, argv);
                }
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

    terminalDestroy();
    return 0;
}

int getArgs(char *line, int *argc, char **argv, FILE *from) {
    // Lettura stringa
    if (fgets(line, MAX_LEN, from) == NULL) return -1;  // Per evitare la ripetizione di comandi in caso venga ricevuto un segnale
    line[strcspn(line, "\n")] = '\0';                   //  Rimuovo eventuali \n dalla fine della stringa per evitare problemi nel parse
    // Parse argomenti
    int pos = 0;
    char *arg = strtok(line, " ");  //  Parse primo parametro
    while (arg != NULL && pos < MAX_ARGC) {
        argv[pos++] = arg;
        arg = strtok(NULL, " ");  //  Parse parametro successivo
    }
    *argc = pos;
    if (from != stdin) usleep(255);  // Per evitare problemi di importazione del file data la lettura veloce. Per maggiori info fare riferimento alla documentazione
    return 0;
}

void printHelp(char *cmd, char *desc) {
    printf("    %-30s%s\n", cmd, desc);
}
