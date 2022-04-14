#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "nodo.h"

#define SH_PARAM_ID 
#define SO_TP_SIZE 1
#define SM_PARAM_ID

/*Semaforo per segnalare che i nodi sono pronti*/
#define ID_READY 0;

int main(int argc, char *argv[])
{
    /*Create transaction pool list*/
    list transaction_pool=NULL;
    printf("\ncreato pid nodo: %d\n",getpid());
    /*Aggiungo transazione da processare*/
     printf("transaction length: %d\n",l_length(transaction_pool));
    if(SO_TP_SIZE<l_length(transaction_pool)){
        printf("Transaction pool is full\n");
        return;
    }l_add_transaction(new_transaction(1,1,2,1,1),&transaction_pool);
   l_add_transaction(new_transaction(4,4,2,1,1),&transaction_pool);
    printf("transaction length: %d\n",l_length(transaction_pool));
    l_print(transaction_pool);
   
    /*coda di messaggi*/
    /* semop con id che punta al semaforo per poter notificare al padre 
    che il nodo ha creato la sua coda di messaggi */
    exit(EXIT_SUCCESS);
}