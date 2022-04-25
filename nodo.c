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

#define SH_PARAM_ID 
#define SO_TP_SIZE atoi(getenv("SO_TP_SIZE")) 
#define SH_NODES_ID atoi(argv[1])
#define SH_SEM_ID atoi(argv[2])
#define NODE_ID atoi(argv[3])
#define REWARD_SENDER -1 

/*Semaforo per segnalare che i nodi sono pronti*/
#define ID_READY 0;

int sem_nodes_id;
int block_reward;
struct timespec timestamp;
node_struct *nodes;

struct sembuf sops;

/*Create transaction pool list*/
list transaction_pool;

int main(int argc, char *argv[])
{    
  
    /* Mi attacco alle memorie condivise */
    nodes = shmat(SH_NODES_ID, NULL, 0);
    TEST_ERROR;
    /*printf("\n mi sono connesso alla memoria condivisa con id: %d\n", SH_NODES_ID);
    stampaStatoMemoria(SH_NODES_ID);*/

    /* Devo crearmi la coda di messaggi */
    nodes[NODE_ID].id_mq = msgget(getpid(), 0600 | IPC_CREAT);
    TEST_ERROR;
    /*printf("ho creato la coda di messaggi id: %d e l'ho inserita dentro la memoria condivisa con id: %d\n", nodes[NODE_ID].id_mq, SH_NODES_ID);
    stampaStatoMemoria(SH_NODES_ID);*/

    /* semop con id che punta al semaforo per poter notificare al padre 
    che il nodo ha creato la sua coda di messaggi */
    sops.sem_num = 0;
    sops.sem_op = 1;
    semop(SH_SEM_ID, &sops, 1);

    clock_gettime(CLOCK_REALTIME, &timestamp);
    TEST_ERROR;
    /*Prelevo dalla coda SO_TP_SIZE-1 transazioni */
    if(SO_TP_SIZE<l_length(transaction_pool)){
        printf("Transaction pool is full\n");
        return 0;
    }

    /*Aggiungo transazione di reward*/
    l_add_transaction(new_transaction(timestamp.tv_nsec,REWARD_SENDER,getpid(), block_reward,0),&transaction_pool);

    msgctl(nodes[NODE_ID].id_mq,0,IPC_RMID);
    TEST_ERROR;
    exit(EXIT_SUCCESS); 
}
