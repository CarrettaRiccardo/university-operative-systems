#define SET_TIME_DELAY "set_delay"
#define SET_TIME_BEGIN "set_begin"
#define SET_TIME_END "set_end"
#define SET_TEMPERATURE "set_temperature"
#define SET_PERC_FILLED "set_filled"

/*************************  Tipi messaggi e parametri *************************/
//BUSY: per segnalare che un dispositivo non può eseguire il comando che riceve
#define BUSY_MSG_TYPE 99

// LIST
#define LIST_MSG_TYPE 0
#define LIST_VAL_ID 0
#define LIST_VAL_LEVEL 1
#define LIST_VAL_STATE 2
#define LIST_VAL_STOP 3

// INFO
#define INFO_MSG_TYPE 1
#define INFO_VAL_ID 0
#define INFO_VAL_LEVEL 1
#define INFO_VAL_STATE 2
#define INFO_VAL_STOP 3
#define INFO_VAL_LABELS 4
// Per passare i valori dei registri nella INFO
#define INFO_VAL_REG_TIME 5
#define INFO_VAL_REG_DELAY 6
#define INFO_VAL_REG_PERC 7
#define INFO_VAL_REG_TEMP 8

// DELETE
#define DELETE_MSG_TYPE 2
#define DELETE_VAL_RESPONSE 0

// LINK
#define LINK_MSG_TYPE 3
#define LINK_VAL_PID 0
#define LINK_VAL_SUCCESS 1
#define LINK_ERROR_NOT_CONTROL -1
#define LINK_ERROR_MAX_CHILD -2

// CLONE (ogni dispositivo manderà valori diversi)
#define CLONE_MSG_TYPE 4

// GET CHILDREN
#define GET_CHILDREN_MSG_TYPE 5
#define GET_CHILDREN_VAL_POS 0
#define GET_CHILDREN_VAL_ID 1

// SWITCH
#define SWITCH_MSG_TYPE 6
#define SWITCH_VAL_LABEL 0
#define SWITCH_VAL_POS 1
#define SWITCH_VAL_SUCCESS 2
#define SWITCH_VAL_DEST 3
#define SWITCH_ERROR_INVALID_VALUE -1

// SET
#define SET_MSG_TYPE 8
#define SET_VAL_LABEL 0
#define SET_VAL_VALUE 1
#define SET_VAL_SUCCESS 2
#define SET_TIMER_STARTED_ON_SUCCESS 2

// TRANSLATE
#define TRANSLATE_MSG_TYPE 7
#define TRANSLATE_VAL_FOUND 0
#define TRANSLATE_VAL_ID 1

// ADD
#define ADD_MSG_TYPE 8
#define ADD_VAL_DEVICE 0
#define ADD_VAL_RESPONSE 0

//Componente TERMINAL
//#define TERMINAL_ADD_RESPONSE 0
//#define TERMINAL_DELETE_RESPONSE 0
#define TERMINAL_DELETE_DEST 0
#define TERMINAL_LINK_DEST 0
//#define TERMINAL_LINK_RESPONSE 0
#define TERMINAL_INFO_DEST 0

/*************************  Tipi interruttori SWITCH  *************************/
#define INVALID_VALUE __INT_MAX__

#define LABEL_LIGHT "light"
#define LABEL_OPEN "open"
#define LABEL_THERM "therm"
#define LABEL_ALL "all"

// In binario per inviare tutto con operazioni AND e OR
#define LABEL_LIGHT_VALUE 1
#define LABEL_OPEN_VALUE 2
#define LABEL_THERM_VALUE 4
#define LABEL_ALL_VALUE 8

#define SWITCH_POS_OFF_LABEL "off"
#define SWITCH_POS_ON_LABEL "on"
#define SWITCH_POS_OFF_LABEL_VALUE 0
#define SWITCH_POS_ON_LABEL_VALUE 1

/*************************  Tipi interruttori SET  *************************/
#define LABEL_DELAY "delay"
#define LABEL_BEGIN "begin"
#define LABEL_END "end"
#define LABEL_PERC "perc"
#define LABEL_TEMP "temp"
#define LABEL_TIME "time"

#define LABEL_DELAY_VALUE 1
#define LABEL_BEGIN_VALUE 2
#define LABEL_END_VALUE 4
#define LABEL_PERC_VALUE 8

/*************************  Tipi dispositivi  *************************/
#define MAX_DEVICE_NAME_LENGTH 20

#define BULB "bulb"
#define FRIDGE "fridge"
#define WINDOW "window"
#define HUB "hub"
#define TIMER "timer"
#define HOME "home"

/*************************  Colori per printf  *************************/
#define C_RESET "\x1b[0m"
#define C_WHITE C_RESET
#define CB_WHITE C_RESET "\x1b[1;37m"
#define C_RED C_RESET "\x1b[31m"     // Normale
#define CB_RED C_RESET "\x1b[1;31m"  // Bold
#define C_GREEN C_RESET "\x1b[32m"
#define CB_GREEN C_RESET "\x1b[1;32m"
#define C_YELLOW C_RESET "\x1b[33m"
#define CB_YELLOW C_RESET "\x1b[1;33m"
#define C_BLUE C_RESET "\x1b[34m"
#define CB_BLUE C_RESET "\x1b[1;34m"
#define C_MAGENTA C_RESET "\x1b[35m"
#define CB_MAGENTA C_RESET "\x1b[1;35m"
#define C_CYAN C_RESET "\x1b[36m"
#define CB_CYAN C_RESET "\x1b[1;36m"
