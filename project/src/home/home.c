#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/constants.h"
#include "../include/utils.h"
#include "./home_lib.c"

/*  Metodi implementati in home_lib  */
void controllerInit(char *file);
void controllerDestroy();
void listDevices();
int addDevice(char *device);
void delDevice(int id);
void linkDevices(int id1, int id2);
int switchDevice(int id, char *label, char *pos);
int setDevice(int id, char *label, char *val);
//void infoDevice(int id);

/* 
   Schema funzionamento all' interno del ciclo while :
    - Eseguo il comando che ricevo ed inoltro la risposta a chi mi ha inviato il comando il quale gestirà i valori di ritorno
*/
int main(int sargc, char **sargv) {
    homeInit(sargv[0], sargv[1]);

    short run = 1;       //  Per bloccare l'esecuzione

    while (run) {
        int result;
        message_t command;
        receiveMessage(&command);

        switch(command.type){
            case LIST_MSG_TYPE: listDevices(); break;
            case ADD_MSG_TYPE:{
                message_t ret = buildAddResponse(command.sender, addDevice(command.text));
                if (sendMessage(ret) == -1)
                    perror("Error: sending message home "); 
            } break; 
            case DEL_MSG_TYPE:{
                message_t ret = buildDeleteResponse(command.sender, delDevice(ret.val[TERMINAL_DELETE_DEST]));
                if (sendMessage(ret) == -1)
                    perror("Error: sending message home "); 
            } break;
            case LINK_MSG_TYPE:{
                message_t ret = buildLinkResponse(command.sender, linkDevice(ret.val[LINK_VAL_PID], ret.val[TERMINAL_LINK_DEST]));
                if (sendMessage(ret) == -1)
                    perror("Error: sending message home "); 
            } break; 
            case SWITCH_MSG_TYPE:{
                message_t ret = buildSwitchResponse(command.sender, switchDevice(ret.vals[SWITCH_VAL_DEST], ret.vals[SWITCH_VAL_LABEL], ret.vals[SWITCH_VAL_POS]));
                if (sendMessage(ret) == -1)
                    perror("Error: sending message home "); 
            } break;  
            case SET_MSG_TYPE:{
                /* TODO: implementare SET
                message_t ret = buildSwitchResponse(command.sender, switchDevice(ret.vals[SWITCH_VAL_DEST], ret.vals[SWITCH_VAL_LABEL], ret.vals[SWITCH_VAL_POS]));
                if(sendMessage(ret) == -1)
                    perror("Error: sending message home "); */
            } break;  
            case INFO_MSG_TYPE:{
                message_t impose_command = buildInfoRequest( ret.vals[TERMINAL_INFO_DEST] );
                impose_command.sender = command.sender; //Mi concedo la licenza di rompere lo schema a cascata di invio messaggi per semplificare la gestione degli stessi. Anzichè far passare tutti i messaggi per home impongo al componente finale di rispondere direttamente a terminal
                if (sendMessage(impose_command) == -1)
                    perror("Error: sending message home "); 
            } break;  
        }


        /**************************************** LIST ********************************************/
        if (strcmp(argv[0], "list") == 0) {
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
            } else {
                delDevice(atoi(argv[1]));
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
            } else {
                linkDevices(atoi(argv[1]), atoi(argv[3]));
            }
        }
        /**************************************** SWITCH ********************************************/
        else if (strcmp(argv[0], "switch") == 0) {
            if (argc != ARGC_SWITCH) {
                printf(CB_RED "Unknown parameters, usage: switch <id> <label> <pos>\n" C_WHITE);
            } else if (!isInt(argv[1])) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                switchDevice(atoi(argv[1]), argv[2], argv[3]);
            }
        }
        /**************************************** SET ********************************************/
        else if (strcmp(argv[0], "set") == 0) {
            if (argc != ARGC_SET) {
                printf(CB_RED "Unknown parameters, usage: set <id> <register> <pos>\n" C_WHITE);
            } else if (!isInt(argv[1])) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
            } else {
                setDevice(atoi(argv[1]), argv[2], argv[3]);
            }
        }
        /**************************************** INFO ********************************************/
        else if (strcmp(argv[0], "info") == 0) {
            if (argc != ARGC_INFO) {
                printf(CB_RED "Unknown parameters, usage: info <id>\n" C_WHITE);
            } else if (!isInt(argv[1])) {
                printf(CB_RED "Error: <id> must be a positive number\n" C_WHITE);
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
    controllerDestroy();
    return 0;
}


