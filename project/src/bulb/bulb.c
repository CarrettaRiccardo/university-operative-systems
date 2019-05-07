#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/constants.h"
#include "../include/ipc.h"

/* Override specifico per il metodo definito in IPC */
message_t buildInfoResponseBulb(const long id, const short stato,
                                const int to, const char *tipo_componente,
                                const long work_time);

int main(int argc, char **argv) {
    const int id = atoi(argv[1]);           // Lettura id da parametro
    short stato = 0;                        // 0 = spenta, 1 = accesa
    unsigned long start_time = time(NULL);  // Tempo accensione lampadina

    while (1) {
        message_t msg;
        int result = receiveMessage(getpid(), &msg);
        if (result == -1) {
            perror("BULB: Errore ricezione");
        } else {
            if (msg.to == -1) continue;  // Messaggio da ignorare (per sessione diversa/altri casi)

            if (strcmp(msg.text, "DIE") == 0) {
                message_t msg = buildDieResponse(msg.sender);
                sendMessage(&msg);
                exit(0);
            } else if (strcmp(msg.text, "INFO") == 0) {
                unsigned long work_time = time(NULL) - start_time;
                message_t m = buildInfoResponseBulb(id, stato, msg.sender, "Bulb", work_time);
                sendMessage(&m);
            } else if (strcmp(msg.text, "TRANSLATE") == 0) {
                message_t m = buildTranslateResponse(id, msg.value1, msg.sender);
                sendMessage(&m);
            } else if (strcmp(msg.text, "LIST") == 0) {  // Caso base per la LIST. value5 = 1 per indicare fine albero
                message_t m = buildListResponse(msg.sender, "Bulb", stato, msg.value1, 1, id);
                sendMessage(&m);
            }
        }
    }
    return 0;
}

message_t buildInfoResponseBulb(const long id, const short stato, const int to, const char *tipo_componente, const long work_time) {
    message_t ret = buildInfoResponse(id, stato, to, tipo_componente);
    ret.value1 = work_time;
    return ret;
}