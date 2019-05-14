int main() {
    return 0;
}

// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

// #include "../include/ipc.h"
// #include "../include/list.h"
// #include "../include/utils.h"

// char *base_dir;
// int id;
// list_t child;  // conterrà al max 1 figlio; uso una list_t perchè è compatibile con i metodi usati per hub

// long begin = 0;  //momento temporale di attivazione
// long end = 0;    //momento temporale di disattivazione

// message_t buildInfoResponseTimer(int to_pid);
// message_t buildListResponseTimer(int to_pid, int lv, short stop);

// // ------ BOZZA ------
// //int switchDevice2(char *id, char *label, char *pos);

// int main(int argc, char **argv) {
//     base_dir = extractBaseDir(argv[0]);
//     id = atoi(argv[1]);
//     child = listInit();
//     if (argc > 2) {  //  Clone del timer
//         int to_clone_pid = atol(argv[2]);
//         message_t request = buildGetChildRequest(to_clone_pid);
//         message_t response;
//         int child_pid;
//         sendMessage(&request);
//         // Linka il figlio del timer clonato a sè stesso
//         receiveMessage(&response);
//         child_pid = response.vals[GET_CHILDREN_VAL_ID];
//         if (child_pid != -1) {
//             doLink(child, child_pid, getppid(), base_dir);
//             message_t ack = buildResponse(to_clone_pid, -1);
//             sendMessage(&ack);
//         }
//         //  Invia la conferma al padre
//         message_t confirm_clone = buildLinkResponse(getppid(), 1);
//         sendMessage(&confirm_clone);
//     }

//     while (1) {
//         message_t msg;
//         if (receiveMessage(&msg) == -1) {
//             perror("TIMER: Error receive message");
//         } else {
//             // ------ BOZZA ------
//             /* UPDATE: ad ogni ricezione di messaggio, aggiorno lo stato del figlio in base a begi e end */
//             // controllo se il tempo di accensione automatica è superato
//             /*printf("%d <= %d\n", begin, time(NULL));
//             if (begin <= time(NULL)) {
//                 // se sì, accendo il figlio
//                 switchDevice2("1", "light", "on");// TODO
//             }*/

//             if (msg.type == INFO_MSG_TYPE) {
//                 message_t m = buildInfoResponseTimer(msg.sender);
//                 sendMessage(&m);
//             } else if (msg.type == SWITCH_MSG_TYPE) {
//                 int success = -1;
//                 if (msg.vals[SWITCH_VAL_POS] != __INT_MAX__) {  // se è un valore valido
//                     switch (msg.vals[SWITCH_VAL_LABEL]) {       // set begin/end/stato del figlio
//                         case LABEL_BEGIN_VALUE:
//                             begin = msg.vals[SWITCH_VAL_POS];
//                             success = 1;
//                             break;
//                         case LABEL_END_VALUE:
//                             end = msg.vals[SWITCH_VAL_POS];
//                             success = 1;
//                             break;
//                         case LABEL_GENERIC_SWITCH_VALUE: /**/ success = 1; break;  // TODO
//                     }
//                 }
//                 // return success or not
//                 message_t m = buildSwitchResponse(msg.sender, success);
//                 sendMessage(&m);
//             } else if (msg.type == LINK_MSG_TYPE) {
//                 if (listCount(child) < 1) {  // se non ha figlio
//                     doLink(child, msg.vals[LINK_VAL_PID], msg.sender, base_dir);
//                 } else {  // non può avere più di un figlio
//                     message_t fail = buildBusyResponse(msg.sender);
//                     fail.vals[LINK_VAL_SUCCESS] = LINK_MAX_CHILD;
//                     sendMessage(&fail);
//                 }
//             } else if (msg.type == DELETE_MSG_TYPE) {
//                 message_t m = buildDeleteResponse(msg.sender);
//                 sendMessage(&m);
//                 exit(0);
//             } else if (msg.type == TRANSLATE_MSG_TYPE) {
//                 message_t m = buildTranslateResponseControl(msg.sender, id, msg.vals[TRANSLATE_VAL_ID], child);
//                 sendMessage(&m);
//             } else if (msg.type == LIST_MSG_TYPE) {  //  Risponde con i propri dati e inoltra la richiesta al figlio
//                 message_t m;
//                 if (listEmpty(child)) {
//                     m = buildListResponseTimer(msg.sender, msg.vals[LIST_VAL_LEVEL], 1);
//                     sendMessage(&m);
//                 } else {
//                     m = buildListResponseTimer(msg.sender, msg.vals[LIST_VAL_LEVEL], 0);
//                     sendMessage(&m);
//                     doListControl(msg.sender, child);
//                 }
//             } else if (msg.type == CLONE_MSG_TYPE) {
//                 int vals[NVAL] = {id, getpid()};
//                 message_t m = buildCloneResponse(msg.sender, TIMER, id, vals, 1);
//                 sendMessage(&m);
//             } else if (msg.type == GET_CHILDREN_MSG_TYPE) {
//                 //  Invio il figlio al processo che lo richiede
//                 message_t m;
//                 node_t *p = *child;
//                 if (p != NULL) {
//                     m = buildGetChildResponse(msg.sender, p->value);
//                     sendMessage(&m);
//                     message_t resp;
//                     receiveMessage(&resp);
//                 }
//                 m = buildGetChildResponse(msg.sender, -1);
//                 sendMessage(&m);
//             } else if (msg.type == DIE_MESG_TYPE) {
//                 //  Rimuovo il mittente di questo messaggio dalla lista del mio figlio
//                 listRemove(child, msg.sender);
//             }
//         }
//     }

//     return 0;
// }

// message_t buildInfoResponseTimer(int sender) {
//     // Stato = Override <-> ??? -- TODO
//     node_t *p = *child;
//     int child_State = 0;  // 0 = figlio spento, 1 = figlio acceso
//     short override = 0;   // 0 = no override, 1 = si override
//     while (p != NULL) {
//         message_t request = buildInfoRequest(p->value);
//         message_t response;
//         if (sendMessage(&request) == -1) {
//             perror("Error sending info request in buildInfoResponseTimer");
//         } else if (receiveMessage(&response) == -1) {
//             perror("Error receiving info response in buildInfoResponseTimer");
//         } else {
//             if (response.type != INFO_MSG_TYPE) {
//                 message_t busy = buildBusyResponse(response.sender);
//                 sendMessage(&busy);
//                 continue;  // Faccio ripartire il ciclo sullo stesso figlio se leggo un messaggio non pertinente
//             }
//             child_State = response.vals[INFO_VAL_STATE];
//         }
//         p = NULL;
//     }
//     message_t ret = buildInfoResponse(sender);
//     char *children_str;
//     switch (child_State) {
//         case 0: children_str = "off"; break;
//         case 1: children_str = "on"; break;
//         case 2: children_str = "off (override)"; break;
//         case 3: children_str = "on (override)"; break;
//     }

//     sprintf(ret.text, "%s, state: %s", TIMER, children_str);
//     ret.vals[INFO_VAL_STATE] = child_State;
//     return ret;
// }

// message_t buildListResponseTimer(int to_pid, int lv, short stop) {
//     message_t ret = buildListResponse(to_pid, id, lv, stop);
//     sprintf(ret.text, "%s %s", TIMER, "");
//     return ret;
// }

// // ------ BOZZA ------
// /**************************************** SWITCH ********************************************/
// /*int switchDevice2(char *id, char *label, char *pos) {
//     printf("Modifico l'interruttore %s di %s su %s ...\n", label, id, pos);
//     int pid = getPidById(child, atoi(id));
//     if (pid == -1) {
//         printf("Error: device with id %s not found\n", id);
//         return;
//     }
//     message_t request = buildSwitchRequest(pid, label, pos);
//     message_t response;

//     // Se i parametri creano dei valori validi
//     if (request.vals[SWITCH_VAL_LABEL] != __INT_MAX__ && request.vals[SWITCH_VAL_POS] != __INT_MAX__) {
//         if (sendMessage(&request) == -1)
//             printf("Errore comunicazione, riprova\n");

//         if (receiveMessage(&response) == -1) {
//             perror("Errore switch\n");
//         } else {
//             if (response.vals[SWITCH_VAL_SUCCESS] != -1) {
//                 printf("Modifica effettuata con successo\n");
//             } else {
//                 printf("Errore nella modifica\n");
//             }
//         }
//     } else {
//         perror("Parametri non corretti o coerenti\n");
//     }
// }*/