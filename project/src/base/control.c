#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/ipc.h"
#include "../include/list.h"
#include "../include/utils.h"

/* Metodi da implemantare nei dispositivi di controllo */
void initData();
void cloneData(char **vals);
int handleSetControl(message_t *msg);
message_t buildInfoResponseControl(int to_pid, int id, char *children_state, char *available_labels, char *registers_vals, int lv, short stop);
message_t buildListResponseControl(int to_pid, int id, int lv, short stop);
message_t buildCloneResponseControl(int to_pid, int id);

/* Gestione figlio eliminato */
void sigchldHandler(int signum);

char *base_dir;
int id;
list_t children;
int max_children_count;  // Numero massimo di figli supportati. -1 = inf

int main(int argc, char **argv) {
    // Inizializzazione
    signal(SIGCHLD, sigchldHandler);

    max_children_count = -1;  // Valore di default. I device specifici possono impostare altri valori
    base_dir = extractBaseDir(argv[0]);
    ipcInit(atoi(argv[1]));
    id = atoi(argv[2]);
    children = listIntInit();

    if (argc <= 3) {
        // Inizializzazione nuovo control device
        initData();
    } else {
        // Inzializzazione control device clonato
        cloneData(argv + 4);  // Salto i parametri [0] (percorso file), [1] (id), [2] (to_clone_pid)
        // Clonazione ricorsiva dei figli
        int to_clone_pid = atol(argv[3]);
        // Linka tutti i figli dell'hub clonato a sè stesso
        message_t request = buildGetChildRequest(to_clone_pid);
        message_t response;
        sendMessage(&request);  // Richiedo tutti i figli di to_clone_pid
        int child_pid;
        do {
            receiveMessage(&response);
            child_pid = response.vals[GET_CHILDREN_VAL_ID];
            if (child_pid != -1) {
                doLink(children, child_pid, base_dir);  // Per ogni figlio di to_clone_pid, effettuo il link di quel device al nuovo device appena clonato
                message_t ack;
                receiveMessage(&ack);  //  Attendo una conferma di avvenuta clonazione dal figlio clonato
                ack.to = to_clone_pid;
                sendMessage(&ack);  // Richiedo prossimo figlio a to_clone_pid
            }
        } while (child_pid != -1);
        //  Invia la conferma al padre di avvenuta clonazione
        message_t confirm_clone = buildLinkResponse(getppid(), 1);
        sendMessage(&confirm_clone);
    }

    // Esecuzione control device
    while (1) {
        message_t msg;
        if (receiveMessage(&msg) == -1) continue;  // Ignoro eventuali errori di ricezione, riprova in automatico dato il do while
        switch (msg.type) {
            case TRANSLATE_MSG_TYPE: {
                message_t m;
                if (id == msg.vals[TRANSLATE_VAL_ID]) {
                    m = buildTranslateResponse(msg.sender, getpid());
                } else {
                    // Inoltro la richiesta ai figli
                    int to_pid = getPidById(children, msg.vals[TRANSLATE_VAL_ID]);
                    m = buildTranslateResponse(msg.sender, to_pid);
                }
                sendMessage(&m);
            } break;

            case SWITCH_MSG_TYPE: {
                // return success or not
                int success = doSwitchChildren(msg.vals[SWITCH_VAL_LABEL], msg.vals[SWITCH_VAL_POS]);
                message_t m = buildSwitchResponse(msg.sender, success);
                sendMessage(&m);
            } break;

            case SET_MSG_TYPE: {
                int success = handleSetControl(&msg);
                message_t m = buildSetResponse(msg.sender, success);
                sendMessage(&m);
            } break;

            case INFO_MSG_TYPE: {
                list_t msg_list = listMsgInit();  // Salvo tutti i messaggi ricevuti dai figli per reinviarli dopo
                int count_on = 0, count_off = 0;
                short override = 0;
                int label_values = 0;
                int registers_values[NVAL] = {0};   // Mantiene la somma dei valori dei registri dei figli
                short registers_count[NVAL] = {0};  // Mantiene il count dei figli con quel registro
                node_t *p = children->head;
                while (p != NULL) {
                    message_t request = buildInfoRequest(*(int *)p->value);
                    message_t response;
                    if (sendMessage(&request) == -1) perror("Error info request in control device");

                    int stop = 0;
                    // Ricevo per ogni figlio tutte le risposte INFO dei sottofigli
                    do {
                        if (receiveMessage(&response) == -1) perror("Error info response in control device");

                        response.to = msg.sender;                        // Cambio il destinatario per rispondere al mittente
                        response.vals[INFO_VAL_LEVEL] += 1;              //  Aumento il valore "livello", serve per l'identazione
                        label_values |= response.vals[INFO_VAL_LABELS];  // Eseguo l'OR bit a bit per avere un intero che rappresenta tutti gli interruttori dipsonibili

                        stop = response.vals[INFO_VAL_STOP];
                        if (stop == 1 && p->next == NULL) {  // Ultimo figlio, imposto lo stop
                            response.vals[INFO_VAL_STOP] = 1;
                        } else {
                            response.vals[INFO_VAL_STOP] = 0;
                        }

                        // Sommo i valori dei registri dei figli, in modo da poter calcolare i registri dell'HUB
                        int i;
                        for (i = INFO_VAL_REG_TIME; i <= INFO_VAL_REG_TEMP; i++) {
                            if (response.vals[i] != INVALID_VALUE) {                                  // Se è un valore valido
                                if (registers_count[i] == 0) registers_values[i] = response.vals[i];  // Se è il primo registro con questo valore
                                if (response.vals[i] > registers_values[i])                           // Salvo il valore massimo tra i registri dei figli
                                    registers_values[i] = response.vals[i];
                                registers_count[i]++;
                            }
                        }

                        listPushBack(msg_list, &response, sizeof(message_t));  // Salvo il messaggio per inviarlo dopo aver calcolato lo stato dell'HUB da quello dei figli
                        switch (response.vals[INFO_VAL_STATE]) {
                            case 0: count_off++; break;  // off
                            case 1: count_on++; break;   // on
                            case 2:                      // off (override)
                                count_off++;
                                override = 1;
                                break;
                            case 3:  // on (override)
                                count_on++;
                                override = 1;
                                break;
                        }
                    } while (stop != 1);  // Se stop = 1 non ho risposte da altri sottofigli da salvare, posso quindi passare al prossimo figlio
                    p = p->next;
                }

                short children_state;
                if (count_on == 0 && count_off == 0)
                    children_state = -1;  // Nessun figlio
                else if (override == 0 && count_on == 0)
                    children_state = 0;  // off
                else if (override == 0 && count_off == 0)
                    children_state = 1;  // on
                else
                    children_state = (count_off >= count_on) ? 2 : 3;  // off (override) / on (override)

                // Lo stato dell'hub è dato dal valore di maggioranza dello stato dei figli
                char *children_str;
                switch (children_state) {
                    case -1: children_str = "(no connected devices)"; break;
                    case 0: children_str = "off"; break;
                    case 1: children_str = "on"; break;
                    case 2: children_str = "off (override)"; break;
                    case 3: children_str = "on (override)"; break;
                }

                // Costruisco la stringa delle label disponibili nel dispositivo di controllo
                char labels_str[64] = "";
                if (label_values != 0) label_values |= LABEL_ALL_VALUE;  // Se ho dei figli mostro anche l'interruttore all
                if (label_values & LABEL_ALL_VALUE) strcat(labels_str, " " LABEL_ALL);
                if (label_values & LABEL_LIGHT_VALUE) strcat(labels_str, " " LABEL_LIGHT);
                if (label_values & LABEL_OPEN_VALUE) strcat(labels_str, " " LABEL_OPEN);
                if (label_values & LABEL_THERM_VALUE) strcat(labels_str, " " LABEL_THERM);
                if (strlen(labels_str) == 0) strcat(labels_str, " (empty)");  // Nel caso non avessi nessun interruttore

                // Calcolo i valori dei registri disponibili e lo setto
                char registers_str[64] = "";
                int i;
                for (i = INFO_VAL_REG_TIME; i <= INFO_VAL_REG_TEMP; i++) {
                    if (registers_count[i] > 0) {
                        int value = registers_values[i];
                        char reg_str[16] = "";
                        if (i == INFO_VAL_REG_TIME) snprintf(reg_str, 16, " " LABEL_TIME "=%ds", value);
                        if (i == INFO_VAL_REG_DELAY) snprintf(reg_str, 16, " " LABEL_DELAY "=%ds", value);
                        if (i == INFO_VAL_REG_PERC) snprintf(reg_str, 16, " " LABEL_PERC "=%d%%", value);
                        if (i == INFO_VAL_REG_TEMP) snprintf(reg_str, 16, " " LABEL_TEMP "=%d°C", value);
                        strcat(registers_str, reg_str);
                    }
                }
                if (strlen(registers_str) == 0) strcat(registers_str, " (empty)");  // Nel caso non avessi nessun registro

                message_t m = buildInfoResponseControl(msg.sender, id, children_str, labels_str, registers_str, msg.vals[INFO_VAL_LEVEL], listEmpty(msg_list));  // Implementazione specifica dispositivo
                m.vals[INFO_VAL_STATE] = children_state;
                m.vals[INFO_VAL_LABELS] = label_values;
                for (i = INFO_VAL_REG_TIME; i <= INFO_VAL_REG_TEMP; i++) m.vals[i] = registers_count[i] > 0 ? registers_values[i] : INVALID_VALUE;
                sendMessage(&m);

                // Invio messaggi ricevuti dai figli al mittente
                node_t *el = msg_list->head;
                while (el != NULL) {
                    sendMessage((message_t *)el->value);
                    el = el->next;
                }
                listDestroy(msg_list);
            } break;

            case LIST_MSG_TYPE: {
                message_t m;
                if (listEmpty(children)) {
                    m = buildListResponseControl(msg.sender, id, msg.vals[LIST_VAL_LEVEL], 1);  // Implementazione specifica dispositivo
                    sendMessage(&m);
                } else {
                    m = buildListResponseControl(msg.sender, id, msg.vals[LIST_VAL_LEVEL], 0);  // Implementazione specifica dispositivo
                    sendMessage(&m);
                    //  Inoltro richiesta LIST ai figli
                    node_t *p = children->head;
                    while (p != NULL) {
                        int son = *(int *)p->value;
                        message_t request = buildListRequest(son);
                        if (sendMessage(&request) == -1)
                            printf("Error sending list control request to pid %d: %s\n", son, strerror(errno));

                        message_t response;
                        int stop = 0;
                        do {
                            // TODO: implementare BUSY globalmente
                            do {  // Se ricevo un messaggio diverso da quello che mi aspetto, rispondo BUSY
                                if (receiveMessage(&response) == -1)
                                    perror("Error receiving list control response");
                                if (response.type != LIST_MSG_TYPE) {
                                    message_t busy = buildBusyResponse(response.sender);
                                    sendMessage(&busy);
                                }
                            } while (response.type != LIST_MSG_TYPE);

                            response.to = msg.sender;            // Cambio il destinatario per rispondere al mittente
                            response.vals[LIST_VAL_LEVEL] += 1;  //  Aumento il valore "livello"
                            stop = response.vals[LIST_VAL_STOP];
                            response.vals[LIST_VAL_STOP] = 0;  //  Tolgo lo stop dalla risposta
                            if (stop == 1 && p->next == NULL) {
                                //  Ultimo figlio, imposto lo stop
                                response.vals[LIST_VAL_STOP] = 1;
                            }
                            sendMessage(&response);
                        } while (stop != 1);
                        p = p->next;
                    }
                }
            } break;

            case LINK_MSG_TYPE: {
                // Controllo sul numero di figli massimo supportato
                if (max_children_count == -1 || listCount(children) < max_children_count) {
                    doLink(children, msg.vals[LINK_VAL_PID], base_dir);
                    //  Attendo una conferma dal figlio clonato e la inoltro al mittente.
                    message_t ack;
                    receiveMessage(&ack);
                    ack.sender = getpid();
                    ack.to = msg.sender;
                    sendMessage(&ack);
                } else {
                    //  Invia l'errore MAX_CHILD al mittente
                    message_t confirm_clone = buildLinkResponse(msg.sender, LINK_ERROR_MAX_CHILD);
                    sendMessage(&confirm_clone);
                }
            } break;

            case CLONE_MSG_TYPE: {
                message_t m = buildCloneResponseControl(msg.sender, id);  // Implementazione specifica dispositivo
                sendMessage(&m);
            } break;

            case GET_CHILDREN_MSG_TYPE: {
                //  Invio tutti i figli al processo che lo richiede
                message_t m;
                node_t *p = children->head;
                while (p != NULL) {
                    m = buildGetChildResponse(msg.sender, *(int *)p->value);
                    sendMessage(&m);
                    receiveMessage(NULL);  // ACK, conferma ricezione dati figlio
                    p = p->next;
                }
                m = buildGetChildResponse(msg.sender, -1);
                sendMessage(&m);
            } break;

            case DELETE_MSG_TYPE: {
                signal(SIGCHLD, NULL);  // Rimuovo l'handler in modo da non interrompere l'esecuzione mentre elimino ricorsivamente i figli
                node_t *p = children->head;
                message_t kill_req, kill_resp;
                while (p != NULL) {
                    kill_req = buildDeleteRequest(*(int *)p->value);
                    sendMessage(&kill_req);
                    receiveMessage(&kill_resp);
                    p = p->next;
                }
                message_t m = buildDeleteResponse(msg.sender);
                sendMessage(&m);
                exit(0);
            } break;
        }
    }
    return 0;
}

void sigchldHandler(int signum) {
    int pid;
    do {
        pid = waitpid(-1, NULL, WNOHANG);
        listRemove(children, &pid);
    } while (pid != -1 && pid != 0);
}

int doSwitchChildren(int label, int pos) {
    int success = -1;
    // Fa lo switch di tutti i figli
    node_t *p = children->head;
    while (p != NULL) {
        message_t m = buildSwitchRequest(*(int *)p->value, label, pos);
        sendMessage(&m);
        message_t resp;
        receiveMessage(&resp);
        if (resp.vals[SWITCH_VAL_SUCCESS] != -1) success = 1;  // Un figlio ha modificato il proprio stato con successo
        p = p->next;
    }
    return success;
}

/*int doSetChildren(int label, int val) {
    int success = -1;
    // Fa il set di tutti i figli
    node_t *p = children->head;
    while (p != NULL) {
        message_t m = buildSetRequest(*(int *)p->value, label, val);
        sendMessage(&m);
        message_t resp;
        receiveMessage(&resp);
        if (resp.vals[SET_VAL_SUCCESS] != -1) success = 1;  // Un figlio ha modificato il proprio registro con successo
        p = p->next;
    }
    return success;
}*/