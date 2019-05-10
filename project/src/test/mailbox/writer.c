// C Program for Message Queue (Writer Process)
#include <errno.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// structure for message queue
struct mesg_buffer {
    int mesg_type;
    char mesg_text[10];
    key_t reponse_key;
} message;

int main() {
    key_t key;
    int msgid;

    // ftok to generate unique key
    key = ftok("progfile", 65);

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    if (msgid == -1) {
        perror("Error ");
        //printf("Errore 1 %d \n",errno);
    }

    printf("Write Data : ");
    gets(message.mesg_text);

    // msgsnd to send message
    int ret = msgsnd(msgid, &message, sizeof(message), 0);

    if (ret == -1) {
        printf("Errore 2 %s \n", strerror(errno));
    }

    // display the message
    printf("Data send is : %s \n", message.mesg_text);

    return 0;
}