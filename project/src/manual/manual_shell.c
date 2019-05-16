#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/constants.h"
#include "../include/utils.h"

#define MAX_LEN 256  //  Massima lunghezza stringa parametri
#define MAX_ARGC 10  //  Massimo numero parametri

#define ARGC_HELP 1
#define ARGC_LIST 1
#define ARGC_ADD 2
#define ARGC_DEL 2
#define ARGC_LINK 4
#define ARGC_SWITCH 4
#define ARGC_SET 4
#define ARGC_INFO 2
#define ARGC_QUIT 1

// PID del controller collegato
int controller_pid;

/*  Lettura e parse parametri */
void getArgs(char *line, int *argc, char **argv);
/*  Print con identazione   */
void printHelp(char *cmd, char *desc);

/*  Metodi implementati in manual   */
void controllerInit();
void controllerDestroy();
void delDevice(int id);
void linkDevices(int id1, int id2);
int switchDevice(int id, char *label, char *pos);
int setRegisterDevice(int id, char *reg, char *val);
void infoDevice(int id);

/* Main */
int main(int sargc, char **sargv) {
    if (sargc < 1) {
        printf("Usage: manual <controller_id>\n");
        exit(0);
    }

    controller_pid = atoi(sargv[1]);

    short run = 1;  //  Per uscire dal while nel caso si scriva "quit"
    char line[MAX_LEN];
    int argc = 0;
    char *argv[MAX_ARGC];
    printf("Type \"help\" for more information.\n\n");

    while (run) {
        printf("> ");
        getArgs(line, &argc, argv);

        /**************************************** HELP ********************************************/
        if (strcmp(argv[0], "help") == 0) {
            printf("Available commands:\n");
            printHelp("help", "Print this page.");
            printHelp("del <id>", "Remove device <id>. If the device is a control device, remove also the linked devices.");
            printHelp("link <id> to <id>", "Link two devices.");
            printHelp("switch <id> <label> <pos>", "Change the value of the switch <label> of the device <id> to <pos>.");
            printHelp("set <id> <register> <value>", "Change the value of the register <register> of the device <id> to <value>.");
            printHelp("info <id>", "Print device <id> info.");
            printHelp("quit", "Close the shell");
        }
        /**************************************** DEL ********************************************/
        else if (strcmp(argv[0], "del") == 0) {
            if (argc != ARGC_DEL) {
                printf("Unknown parameters, usage: del <id>\n");
            } else if (!isInt(argv[1])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                delDevice(atoi(argv[1]));
            }
        }
        /**************************************** LINK ********************************************/
        else if (strcmp(argv[0], "link") == 0) {
            if (argc != ARGC_LINK || strcmp(argv[2], "to") != 0) {
                printf("Unknown parameters, usage: link <id> to <id>\n");
            } else if (!isInt(argv[1]) || !isInt(argv[3])) {
                printf("Error: <id> must be a positive number\n");
            } else if (strcmp(argv[1], argv[3]) == 0) {
                printf("Error: cannot link a device to itself\n");
            } else {
                linkDevices(atoi(argv[1]), atoi(argv[3]));
            }
        }
        /**************************************** SWITCH ********************************************/
        else if (strcmp(argv[0], "switch") == 0) {
            if (argc != ARGC_SWITCH) {
                printf("Unknown parameters, usage: switch <id> <label> <pos>\n");
            } else if (!isInt(argv[1])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                switchDevice(atoi(argv[1]), argv[2], argv[3]);
            }
        }
        /**************************************** SET ********************************************/
        else if (strcmp(argv[0], "set") == 0) {
            if (argc != ARGC_SWITCH) {
                printf("Unknown parameters, usage: set <id> <register> <value>\n");
            } else if (!isInt(argv[1])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                switchDevice(atoi(argv[1]), argv[2], argv[3]);
            }
        }
        /**************************************** INFO ********************************************/
        else if (strcmp(argv[0], "info") == 0) {
            if (argc != ARGC_INFO) {
                printf("Unknown parameters, usage: info <id>\n");
            } else if (!isInt(argv[1])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                infoDevice(atoi(argv[1]));
            }
        }
        /**************************************** QUIT ********************************************/
        else if (strcmp(argv[0], "quit") == 0) {
            run = 0;
        } else if (argc > 0) {
            printf("Unknown command, type \"help\" to list all the supported commands\n");
        }
    }
    return 0;
}

void getArgs(char *line, int *argc, char **argv) {
    // Lettura stringa
    fgets(line, MAX_LEN, stdin);
    line[strcspn(line, "\n")] = '\0';  //  Rimuovo eventuali \n dalla fine della stringa per evitare problemi nel parse
    // Parse argomenti
    int pos = 0;
    char *arg = strtok(line, " ");  //  Parse primo parametro
    while (arg != NULL && pos < MAX_ARGC) {
        argv[pos++] = arg;
        arg = strtok(NULL, " ");  //  Parse parametro successivo
    }
    *argc = pos;
}

void printHelp(char *cmd, char *desc) {
    printf("  %-28s%s\n", cmd, desc);
}