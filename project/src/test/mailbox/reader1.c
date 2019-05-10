// C Program for Message Queue (Reader Process)
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// structure for message queue
struct mesg_buffer {
    int mesg_type;
    char mesg_text[10];
    time_t session;
} message;

int main(int argc, char **argv) {
    key_t key;
    //int sessione = (int) atoi(argv[0]);
    int msgid;

    // ftok to generate unique key
    key = ftok("progfile", 65);

    msgid = msgget(key, 0666 | IPC_CREAT);
    //printf("La sessione è %d\n",sessione);
    // msgrcv to receive message
    while (1) {
        int ret = msgrcv(msgid, &message, sizeof(message), 1, 0);

        if (ret == -1) {
            printf("Errore %d", ret);
        }

        // display the message
        printf("Data Received is : %s con sessione = %d \n", message.mesg_text, message.session);
    }

    // to destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);

    return 0;
}
