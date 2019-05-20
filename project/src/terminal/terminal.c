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
#define ARGC_SWITCH 4
#define ARGC_SET 4
#define ARGC_INFO 2
#define ARGC_QUIT 1

/*  Lettura e parse parametri */
int getArgs(char *line, int *argc, char **argv);
/*  Print con identazione   */
void printHelp(char *cmd, char *desc);

/*  Metodi implementati nel controller   */
int componentInit();

void listDevices();
int addDevice();
int delDevice(int id);
int linkDevices(int id1, int id2);
int switchDevice(int id, char *label, char *pos);
int setDevice(int id, char *label, char *val);
void infoDevice(int id);

/* 
   Schema funzionamento all' interno del ciclo while :
    - Ricevo un comando da tastiera, costruisco un messaggio a seconda della sua sintassi specifica ed lo inoltro al componente HOME che si occupeà di eseguire tale comando.
      Resta in attesa di risposta per fornire feedback all' utilizzatore
*/
int main(int sargc, char **sargv) {
    ipcInit(getMq(getpid()));  // Inizializzo componenti comunicazione
    componentInit();

    short run = 1;       //  Per uscire dal while nel caso si scriva "quit"
    int print_sign = 1;  // Per identificare quando stampare >. Serve per evitare duplicati nel caso venga ricevuto un segnale da una shell manuale
    char line[MAX_LEN];
    int argc = 0;
    char *argv[MAX_ARGC];

    printf("Controller identifier (use this for manual commands): " CB_WHITE "%d\n" C_WHITE, getpid());
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
            printHelp("list", "List the installed devices.");
            printHelp("add <device>", "Add <device> to the system.");
            printHelp("del <id>", "Remove device <id>. If the device is a control device, remove also the linked devices.");
            printHelp("link <id> to <id>", "Link two devices.");
            printHelp("switch <id> <label> <pos>", "Change the value of the switch <label> of the device <id> to <pos>.");
            printHelp("set <id> <register> <value>", "Change the value of the register <register> of the device <id> to <value>.");
            printHelp("info <id>", "Print device <id> info.");
            printHelp("quit", "Close the controller and kill all processes.");
        }
        /**************************************** LIST ********************************************/
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
                else if (strcmp(argv[1], HUB) == 0)
                    result = addDevice(HUB);
                else if (strcmp(argv[1], TIMER) == 0)
                    result = addDevice(TIMER);
                else
                    printf(CB_RED "Unknown device, supported devices: bulb, fridge, window, hub, timer\n" C_WHITE);

                if (result == -1)
                    perror(CB_RED "Error while adding device" C_WHITE);
                else if (result != 0)  //  Se ho aggiunto un device supportato
                    printf(CB_GREEN "Device added with id (%d)" C_WHITE "\nNow it's disconnected from the system. To connect the device run " CB_YELLOW "link %d to 0\n" C_WHITE, result, result);
            }
        }
        /**************************************** DEL ********************************************/
        else if (strcmp(argv[0], "del") == 0) {
            if (argc != ARGC_DEL) {
                printf(CB_RED "Unknown parameters, usage: del <id>\n" C_WHITE);
            } else if (!isInt(argv[1])) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else if (atoi(argv[1]) == 0) {
                printf(CB_RED "Error: the controller cannot be deleted\n" C_WHITE);
                return;
            }else{
                int id = atoi(argv[1]);
                int result = delDevice(id);
                switch(result){
                    case 1: printf(CB_GREEN "Device %d deleted\n" C_WHITE, id); break;
                    case 0: printf(CB_RED "Error: id = 0 delDevice home_lib\n" C_WHITE); break;
                    case -1: printf(CB_RED "Device %d not found in the system\n" C_WHITE, id); break;
                    case -2: printf(CB_RED "Error sending message, problem with IPC\n" C_WHITE); break;
                    case -3: printf(CB_RED "Error receiving message, problem with IPC\n" C_WHITE, id, result); break;
                    default: printf(CB_RED "Device %d not deleted for an internal error (cod err %d)\n" C_WHITE, id, result); break;
                }   
            }
        }
        /**************************************** LINK ********************************************/
        else if (strcmp(argv[0], "link") == 0) {
            if (argc != ARGC_LINK || strcmp(argv[2], "to") != 0) {
                printf(CB_RED "Unknown parameters, usage: link <id> to <id>\n" C_WHITE);
            } else if (!isInt(argv[1]) || !isInt(argv[3])) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else if (strcmp(argv[1], argv[3]) == 0) {
                printf(CB_RED "Error: cannot link a device to itself\n" C_WHITE);
            } else if(atoi(argv[1]) == 0 ){
                printf(CB_RED "Error: cannot connect the controller to other devices\n" C_WHITE);
            } else if(atoi(argv[3]) < 0 ){
                printf(CB_RED "Error: second id not valid\n" C_WHITE);
            }else {
                int id1 = atoi(argv[1]), id2 = atoi(argv[3]);
                int result = linkDevices(id1, id2);

                switch(result){
                    case 1: printf(CB_GREEN "Device (%d) linked to (%d)\n" C_WHITE, id1, id2); break;
                    case 0: printf(CB_RED "Error: source id must be > 0\n" C_WHITE, id1); break;
                    case -1: printf(CB_RED "Error: device with id (%d) not found\n" C_WHITE, id1); break;
                    case -2: printf(CB_RED "Error: device with id (%d) not found\n" C_WHITE, id2); break;
                    case -3: perror("Error sending message link "); break;
                    case -4: perror("Error reading message link "); break;
                    case -5: printf(CB_RED "Error: cycle identified, (%d) is a child of (%d)\n" C_WHITE, id2, id1); break;
                    case -6: printf(CB_RED "Error: the device with id (%d) is not a control device\n" C_WHITE, id2); break;
                    case -7: printf(CB_RED "Error: the device with id (%d) already has a child\n" C_WHITE, id2); break;
                }
            }

            /*
            TODO: Destro posso cancellare questo codice ?
            if (id1 == 0)
            {
                
                return;
            }*/
        }
        /**************************************** SWITCH ********************************************/
        else if (strcmp(argv[0], "switch") == 0) {
            if (argc != ARGC_SWITCH) {
                printf(CB_RED "Unknown parameters, usage: switch <id> <label> <pos>\n" C_WHITE);
            } else if (!isInt(argv[1])) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                int id = atoi(argv[1]); //il primo argomento è l'id del componente
                int result = switchDevice(id, argv[2], argv[3]);
                switch(result){
                    case 1: printf(CB_GREEN "Switch executed\n" C_WHITE); break;
                    case -1: printf(CB_RED "Error: device with id (%d) not connected to the controller\n" C_WHITE, id); break;
                    case -2: printf(CB_RED "Error: device with id (%d) not found\n" C_WHITE, id); break;
                    case -3: printf(CB_RED "Error: invalid label \"%s\"\n" C_WHITE, argv[2]); break;
                    case -4: printf(CB_RED "Error: invalid pos value \"%s\" for label \"%s\". It must be a number between -30°C and 15°C \n" C_WHITE, argv[3], argv[2]); break;
                    case -5: printf(CB_RED "Error: invalid pos value \"%s\" for label \"%s\"" C_WHITE, argv[3], argv[2]); break;
                    case -6: printf(CB_RED "The label \"%s\" is not supported by the device (%d)\n" C_WHITE, argv[2], id); break;
                }
            }
        }
        /**************************************** SET ********************************************/
        else if (strcmp(argv[0], "set") == 0) {
            if (argc != ARGC_SET) {
                printf(CB_RED "Unknown parameters, usage: set <id> <register> <pos>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) == 0 ) {
                printf(CB_RED "Error: <id> must be > 0\n" C_WHITE);
            } else {
                int id = atoi(argv[1]);
                int result = setDevice(id, argv[2], argv[3]);
                switch(result){
                    case 1: printf(CB_GREEN "Set executed (a timer was started)\n" C_WHITE); break;   
                    case 2: printf(CB_GREEN "Set executed\n" C_WHITE); break;
                    case -1: printf(CB_RED "Error: device with id (%d) not found\n" C_WHITE, id); break;
                    case -2: printf(CB_RED "Error: invalid register \"%s\"\n" C_WHITE, argv[2]); break;
                    case -3: printf(CB_RED "Error: invalid value \"%s\" for register \"%s\"\n" C_WHITE, argv[2], argv[3]); break;
                    case -4: printf(CB_RED "The register \"%s\" is not supported by the device (%d)\n" C_WHITE, argv[2], id); break;
                    case -5: printf(CB_RED "The value $d is not an integer value (required)\n" C_WHITE, argv[2], id); break;
                }
            }
        }
        /**************************************** INFO ********************************************/
        else if (strcmp(argv[0], "info") == 0) {
            if (argc != ARGC_INFO) {
                printf(CB_RED "Unknown parameters, usage: info <id>\n" C_WHITE);
            } else if (!isInt(argv[1]) || atoi(argv[1]) == 0 ) {
                printf(CB_RED "Error: <id> must be > 0\n" C_WHITE);
            } else {
                infoDevice(atoi(argv[1]));
            }
        }
        /**************************************** QUIT ********************************************/
        else if (strcmp(argv[0], "quit") == 0) {
            run = 0;
        } else if (argc > 0) {
            printf(CB_RED "Unknown command, type \"help\" to list all the supported commands\n" C_WHITE);
        }
    }
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
