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

#include "master.h"
#include "nodo.h"

#define SH_PARAM_ID 
#define SO_TP_SIZE atoi(getenv("SO_TP_SIZE")) 
#define SH_NODES_ID atoi(argv[1])
#define NODE_ID atoi(argv[3])

#define TEST_ERROR                                 \
    if (errno)                                     \
    {                                              \
        fprintf(stderr,                            \
                "%s:%d: PID=%5d: Error %d (%s)\n", \
                __FILE__,                          \
                __LINE__,                          \
                getpid(),                          \
                errno,                             \
                strerror(errno));                  \
        errno = 0;                                 \
        fflush(stderr);                            \
    }

/*Semaforo per segnalare che i nodi sono pronti*/
#define ID_READY 0;

node_struct *nodes;

int main(int argc, char *argv[])
{
    /* Mi attacco alle memorie condivise */
    printf("AAAAA %d",SH_NODES_ID);
    nodes = shmat(SH_NODES_ID, NULL, SHM_RDONLY);
    TEST_ERROR;

    printf("MADONNA PUTTANA %d", nodes[NODE_ID].pid);

    /* Devo crearmi la coda di messaggi */

    /* semop con id che punta al semaforo per poter notificare al padre 
    che il nodo ha creato la sua coda di messaggi */

    /*Create transaction pool list*/
    list transaction_pool=NULL;
    printf("\ncreato pid nodo: %d\n",getpid());
    /*Aggiungo transazione da processare*/
    l_add_transaction(new_transaction(1,1,2,1,1),&transaction_pool);
    printf("transaction length: %d",l_length(transaction_pool));
    if(SO_TP_SIZE<l_length(transaction_pool)){
        printf("Transaction pool is full\n");
        return;
    }
    
    exit(EXIT_SUCCESS);
}