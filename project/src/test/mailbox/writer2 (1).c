//ps -o  uid,pid,ppid,command -h          (Per elenco processi con ppid)
/*
Se il figlio invia DIE $(pid_processo_morto) qualcosa è andato storto ed è morto. Allora lo elimino dalla lista (da fare).

Siccome le mailbox persistono anche quando i processi muoiono, per evitare che un processo legga messaggi vecchi di un' altra sessione (molto improbabile poichè i pid sono progessivi) utilizzo il timestamp di sistema per marchiare i messaggi. Se sessione_messaggio != sessione_attuale messaggio da scartare (mesg_type = -1)
*/
#include <stdio.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <errno.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
  
#define MAXMSG 15 
#define KEYFILE "progfile"

typedef struct msg { 
    long mesg_type; 
    char text[MAXMSG];
    short int valore;
    bool attivo; 
    time_t session;
}Message;


short int sendMessage(const int mqid,int to,char msg[MAXMSG],const time_t session){
        if(to == 0) {
		printf("Destinatario invalido\n");	
		return -1;
	}
	Message m = { .mesg_type = to, .session = session};
	strcpy(m.text,msg);

	int ret = msgsnd(mqid, &m, sizeof(Message), 0); 

	if(ret == -1){
		printf("Errore invio da controller %s \n",strerror(errno));	
		return -1;
	}
	return 1; 
}

//TODO: Se leggo DIE... processo morto
Message receiveMessage(const int mqid){
	Message ret;
	int reader = getpid(); //ascolto i messaggi indirizzati a me
	int error = msgrcv(mqid, &ret, sizeof(Message), reader, 0); 

	if(error == -1)
		printf("Errore ricezione %d",error);

        if(strlen(ret.text) > 3 && ret.text[0] == 'D' && ret.text[0] == 'i' && ret.text[0] == 'e'){ //TODO: processo figlio deceduto. Il messaggio viene scartato ma lo elimino dalla lista
		ret.mesg_type = -1;	
		//TODO: elimina nodo lista. pid preso da uno split su 'ret.text'
	}

	if(ret.session != current_session) //messaggio di una sessione precedente rimasto in memoria
		ret.mesg_type = -1;
        return ret;
}

key_t getKey(){
	key_t ret = ftok(KEYFILE, 65);
	if(ret == -1){ //TODO :da verirficare
		printf("Errore ottenimento id");
		exit (1);
	}
        return ret;
}

int getMq(){
        const key_t key = getKey();  //creo id per mailbox
	int ret = msgget(key, 0666 | IPC_CREAT); //mi "collego" alla mq
	if(ret == -1){ //TODO :da verirficare
		printf("Errore connessione mq");
		exit (1);
	}
        return ret;
}

void closeMq(const int id){
	if(msgctl(id, IPC_RMID, NULL) == -1){
		printf("Errore chiusura mq");
		exit (1);
	}
}
  
int main() 
{ 
    const int mqid = getMq();
    const time_t current_session = time(NULL);

    char *cmd2 = "./reader2";
    char *args[3];
    args[0] = "./reader2";  //per convenzione va ripetuto
    args[1] = (char*) malloc( (MAXMSG+1)*sizeof(char) );
    args[2] = NULL;   //obbligatorio NULL finale

    sprintf(args[1], "%d" , current_session); //converto time_t in stringa

    int f = fork();
    if(f == 0){
	 if(execvp(cmd2, args) == -1) perror("Error exec ");
    }
    else if(f == -1) perror("Error fork ");
    	
    int to = 0;
    while(1){
	    printf("Inserisci destinatario : "); 
	    scanf("%d",&to);	    
	    
	    if( sendMessage(mqid,to,"Ehi, ciao!",current_session) > 0){
		printf("Data send \nWaiting response..\n"); 
	        Message message = receiveMessage(mqid);		 
                printf("Risposta is : %s (%d)\n",message.text,message.session); 
	    }	    
	    
    }
  
    closeMq(mqid);     
    return 0; 
}
