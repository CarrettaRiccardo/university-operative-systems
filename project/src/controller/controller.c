#include <stdlib.h>
#include "../include/list.h"

list_t l;
int last_id;

/*  Inizializza le variabili del controller   */
void controller_init() {
    list_init(l);
    last_id = 0;
}

/*  Aggiunge un dispositivo al controller in base al tipo specificato   */
int add_device(char *file) {
    int id = ++last_id;  //  Ottengo il nuovo id del device
    int pid = fork();
    if (pid == 0) {  //  Figlio
        char *args = {file, id, NULL};
        execvp(args[0], args);
    } else {  //  Padre
        if (pid != -1) list_push(l, pid);
        return pid;
    }
}

int add_bulb(int id) {
    return add_device("./bulb");
}

int add_fridge(int id) {
    return add_device("./fridge");
}

int add_window(int id) {
    return add_device("./window");
}

int add_hub(int id) {
    return add_device("./hub");
}

int add_timer(int id) {
    return add_device("./timer");
}
