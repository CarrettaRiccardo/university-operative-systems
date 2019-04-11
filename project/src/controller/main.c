#include <stdio.h>
#include <string.h>

#define MAX_LEN 256
#define MAX_LEN_PARAMETER 70

#define N_PARAMETERS_ADD 1
#define N_PARAMETERS_DEL 1
#define N_PARAMETERS_LINK 3
#define N_PARAMETERS_SWITCH 3
#define N_PARAMETERS_INFO 1

int main(int argc, char **argv) {
    // run program until exit is called
    short run = 1;
    // char defined to split the command input string (must be array [])
    char splitChar[] = " ";
    // up to 256 characters in input supported
    char input[MAX_LEN];
    // always listen for a command
    while (run){
        printf("\nInserire Comando: ");
        fgets(input, sizeof(input), stdin);

        // lettura primo parametro (nome comando)
        char *ptr = strtok(input, splitChar);
        
        // per poter tener traccia di quale eventuale parametro si sta leggendo e non leggerne oltre se ce ne sono
        int parameter = 0;

		// ----- LIST -----
        if (strcmp(ptr, "list\n") == 0) {
        	// esecuzione
        	printf("-- Exec LIST");
        }
		// ----- ADD -----
        else if (strcmp(ptr, "add") == 0 || strcmp(ptr, "add\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_ADD][MAX_LEN_PARAMETER];
            // parametri successivi
            while(ptr != NULL && parameter < N_PARAMETERS_ADD)
            {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
	                // alla prima iterazione copio il parametro
	                strcpy(param[parameter++], ptr);
	                // passo al parametro successivo, ma me ne aspetto solo uno per questo comando
				}
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_ADD || strcmp(param[N_PARAMETERS_ADD - 1], "\n") == 0){
                printf("Parametri per il comando 'add X' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_ADD);
            }
            else{
            	// esecuzione
            	printf("-- Exec ADD con parametro %s", param[0]);
            }
        }
		// ----- DEL -----
        else if (strcmp(ptr, "del") == 0 || strcmp(ptr, "del\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_DEL][MAX_LEN_PARAMETER];
            // parametri successivi
            while(ptr != NULL && parameter < N_PARAMETERS_DEL)
            {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
	                // alla prima iterazione copio il parametro
	                strcpy(param[parameter++], ptr);
	                // passo al parametro successivo, ma me ne aspetto solo uno per questo comando
				}
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_DEL || strcmp(param[N_PARAMETERS_DEL - 1], "\n") == 0){
                printf("Parametri per il comando 'del X' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_DEL);
            }
            else{
            	// esecuzione
            	printf("-- Exec DEL con parametro %s", param[0]);
            }
        }
		// ----- LINK -----
        else if (strcmp(ptr, "link") == 0 || strcmp(ptr, "link\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_LINK][MAX_LEN_PARAMETER];
            // parametri successivi
            while(ptr != NULL && parameter < N_PARAMETERS_LINK)
            {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
	                // alla prima iterazione copio il parametro
	                strcpy(param[parameter++], ptr);
	                // passo al parametro successivo
				}
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_LINK || strcmp(param[1], "to") != 0 || strcmp(param[N_PARAMETERS_LINK - 1], "\n") == 0){
                printf("Parametri per il comando 'link X to Y' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_LINK);
            }
            else{
            	// esecuzione
            	printf("-- Exec LINK con parametri %s %s %s", param[0], param[1], param[2]);
            }
        }
		// ----- SWITCH -----
        else if (strcmp(ptr, "switch") == 0 || strcmp(ptr, "switch\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_SWITCH][MAX_LEN_PARAMETER];
            // parametri successivi
            while(ptr != NULL && parameter < N_PARAMETERS_SWITCH)
            {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
	                // alla prima iterazione copio il parametro
	                strcpy(param[parameter++], ptr);
	                // passo al parametro successivo
				}
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_SWITCH || strcmp(param[N_PARAMETERS_SWITCH - 1], "\n") == 0){
                printf("Parametri per il comando 'switch X Y Z' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_SWITCH);
            }
            else{
            	// esecuzione
            	printf("-- Exec SWITCH con parametri %s %s %s", param[0], param[1], param[2]);
            }
        }
		// ----- INFO -----
        else if (strcmp(ptr, "info") == 0 || strcmp(ptr, "info\n") == 0) {
            // nomi dei parametri dopo il comando
            char param[N_PARAMETERS_INFO][MAX_LEN_PARAMETER];
            // parametri successivi
            while(ptr != NULL && parameter < N_PARAMETERS_INFO)
            {
                ptr = strtok(NULL, splitChar);
                if (ptr != NULL) {
	                // alla prima iterazione copio il parametro
	                strcpy(param[parameter++], ptr);
	                // passo al parametro successivo, ma me ne aspetto solo uno per questo comando
				}
            }
            // se non trova un parametro richiesto, mostra un errore
            if (parameter < N_PARAMETERS_INFO || strcmp(param[N_PARAMETERS_INFO - 1], "\n") == 0){
                printf("Parametri per il comando 'info X' non soddisfacenti: atteso/i %d argomento/i", N_PARAMETERS_INFO);
            }
            else{
            	// esecuzione
            	printf("-- Exec INFO con parametro %s", param[0]);
            }
        }
        else if (strcmp(ptr, "exit\n") == 0) {
            printf("-- exec EXIT");
            run = 0;
        }
        else {
            printf("Comando non riconosciuto");
        }
    }

    return 0;
}