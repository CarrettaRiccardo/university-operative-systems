#define SET_TIME_DELAY "set_delay"
#define SET_TIME_BEGIN "set_begin"
#define SET_TIME_END "set_end"
#define SET_TEMPERATURE "set_temperature"
#define SET_PERC_FILLED "set_filled"

/*************  Tipi messaggi e parametri *************/
// LIST
#define LIST_MSG_TYPE 0
#define LIST_VAL_ID 0
#define LIST_VAL_LEVEL 1
#define LIST_VAL_STATE 2
#define LIST_VAL_STOP 3

// INFO
#define INFO_MSG_TYPE 1
#define INFO_VAL_STATE 0

// DELETE
#define DELETE_MSG_TYPE 2

// LINK
#define LINK_MSG_TYPE 3
#define LINK_VAL_PID 0
#define LINK_VAL_SUCCESS 1

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

// TRANSLATE
#define TRANSLATE_MSG_TYPE 7
#define TRANSLATE_VAL_FOUND 0
#define TRANSLATE_VAL_ID 1
//  Parametri messaggi

//  Tipi interruttori
#define LABEL_LIGHT "light"
#define LABEL_OPEN "open"
#define LABEL_TERM "therm"
#define LABEL_LIGHT_VALUE 0
#define LABEL_OPEN_VALUE 1
#define LABEL_TERM_VALUE 2
#define SWITCH_POS_OFF "off"
#define SWITCH_POS_ON "on"
#define SWITCH_POS_OFF_VALUE 0
#define SWITCH_POS_ON_VALUE 1

//  Tipit dispositivo
#define CONTROLLER "CONTROLLER"
#define CONTROL_DEVICE "CONTROL_DEVICE"
#define NORMAL_DEVICE "NORMAL_DEVICE"

//  Dispositivi
#define MAX_DEVICE_NAME_LENGTH 20

#define BULB "bulb"
#define FRIDGE "fridge"
#define WINDOW "window"
#define HUB "hub"
#define TIMER "timer"
