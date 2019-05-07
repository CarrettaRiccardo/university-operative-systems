#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/utils.h"

#define MAX_LEN 256  //  Massima lunghezza stringa parametri
#define MAX_ARGC 10  //  Massimo numero parametri

#define ARGC_HELP 1
#define ARGC_LIST 1
#define ARGC_ADD 2
#define ARGC_DEL 2
#define ARGC_LINK 4
#define ARGC_SWITCH 4
#define ARGC_INFO 2
#define ARGC_QUIT 1

/*  Lettura e parse parametri */
void get_args(char *line, int *argc, char **argv);
/*  Print con identazione   */
void print_help(char *cmd, char *desc);

/* Main */
int main(int sargc, char **sargv) {
    controller_init(sargv[0]);

    short run = 1;  //  Per uscire dal while nel caso si scriva "quit"
    char line[MAX_LEN];
    int argc = 0;
    char *argv[MAX_ARGC];
    printf("Type \"help\" for more information.\n\n");

    while (run) {
        printf("> ");
        get_args(line, &argc, argv);

        /**************************************** HELP ********************************************/
        if (strcmp(argv[0], "help") == 0) {
            printf("Available commands:\n");
            print_help("help", "Print this page.");
            print_help("list", "List the installed devices.");
            print_help("add <device>", "Add <device> to the system.");
            print_help("del <id>", "Remove device <id>. If the device is a control device, remove also the linked devices.");
            print_help("link <id> to <id>", "Link two devices.");
            print_help("switch <id> <label> <pos>", "Change the value of the switch <label> of the device <id> to <pos>.");
            print_help("info <id>", "Print device <id> info.");
            print_help("quit", "Close the controller and kill all processes.");
        }
        /**************************************** LIST ********************************************/
        else if (strcmp(argv[0], "list") == 0) {
            if (argc != ARGC_LIST) {
                printf("Unknown parameters, usage: list\n");
            } else {
                list_devices();
            }
        }
        /**************************************** ADD ********************************************/
        else if (strcmp(argv[0], "add") == 0) {
            if (argc != ARGC_ADD) {
                printf("Unknown parameters, usage: add <device>\n");
            } else {
                int result = 0;
                if (strcmp(argv[1], "bulb") == 0)
                    result = add_bulb();
                else if (strcmp(argv[1], "fridge") == 0)
                    result = add_fridge();
                else if (strcmp(argv[1], "window") == 0)
                    result = add_window();
                else if (strcmp(argv[1], "hub") == 0)
                    result = add_hub();
                else if (strcmp(argv[1], "timer") == 0)
                    result = add_timer();
                else
                    printf("Unknown device, supported devices: bulb, fridge, window, hub, timer\n");

                if (result == -1)
                    perror("Error adding device");
                else if (result != 0)  //  Se ho aggiunto un device supportato
                    printf("Device added\n");
            }
        }
        /**************************************** DEL ********************************************/
        else if (strcmp(argv[0], "del") == 0) {
            if (argc != ARGC_DEL) {
                printf("Unknown parameters, usage: del <id>\n");
            } else if (!is_int(argv[1])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                del_device(argv[1]);
            }
        }
        /**************************************** LINK ********************************************/
        else if (strcmp(argv[0], "link") == 0) {
            if (argc != ARGC_LINK || strcmp(argv[2], "to") != 0) {
                printf("Unknown parameters, usage: link <id> to <id>\n");
            } else if (!is_int(argv[1]) || !is_int(argv[3])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                link_devices(argv[1], argv[3]);
            }
        }
        /**************************************** SWITCH ********************************************/
        else if (strcmp(argv[0], "switch") == 0) {
            if (argc != ARGC_SWITCH) {
                printf("Unknown parameters, usage: switch <id> <label> <pos>\n");
            } else if (!is_int(argv[1])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                switch_device(argv[1], argv[2], argv[3]);
            }
        }
        /**************************************** INFO ********************************************/
        else if (strcmp(argv[0], "info") == 0) {
            if (argc != ARGC_INFO) {
                printf("Unknown parameters, usage: info <id>\n");
            } else if (!is_int(argv[1])) {
                printf("Error: <id> must be a positive number\n");
            } else {
                info_device(argv[1]);
            }
        }
        /**************************************** QUIT ********************************************/
        else if (strcmp(argv[0], "quit") == 0) {
            run = 0;
        } else if (argc > 0) {
            printf("Unknown command, type \"help\" to list all the supported commands\n");
        }
    }
    controller_destroy();
    return 0;
}

void get_args(char *line, int *argc, char **argv) {
    /*  Lettura stringa */
    fgets(line, MAX_LEN, stdin);
    line[strcspn(line, "\n")] = '\0';  //  Rimuovo eventuali \n dalla fine della stringa per evitare problemi nel parse
    /*  Parse args  */
    int pos = 0;
    char *arg = strtok(line, " ");  //  Parse primo parametro
    while (arg != NULL && pos < MAX_ARGC) {
        argv[pos++] = arg;
        arg = strtok(NULL, " ");  //  Parse parametro successivo
    }
    *argc = pos;
}

void print_help(char *cmd, char *desc) {
    printf("  %-28s%s\n", cmd, desc);
}