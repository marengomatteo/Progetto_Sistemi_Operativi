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
#include <time.h>

#include "master.h"
#include "nodo.h"

#define REWARD_SENDER -1 
#define SH_PARAM_ID 
#define SO_TP_SIZE 8
#define SH_NODES_ID atoi(argv[2])
#define SH_SEM_ID
#define NODE_ID atoi(argv[4])
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
int sem_nodes_id;
int block_reward;
struct timespec timestamp;
node_struct *nodes;
/*Create transaction pool list*/
list transaction_pool=NULL;

int main(int argc, char *argv[])
{
    clockid_t clock_id=CLOCK_MONOTONIC;
struct timespec *res;

int time=clock_gettime (clock_id, res);
printf("%d\n",clock_id);
printf("%d\n",time);
    /* Mi attacco alle memorie condivise */
    /*nodes = shmat(SH_NODES_ID, NULL, SHM_RDONLY);
    TEST_ERROR;*/

    /* Devo crearmi la coda di messaggi */
    /*nodes[NODE_ID].id_mq = msgget(getpid(), 0600 | IPC_CREAT);*/

    /* semop con id che punta al semaforo per poter notificare al padre 
    che il nodo ha creato la sua coda di messaggi */
    /*sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    semop(sem_nodes_id, &sops, 1);*/
    printf("\ncreato pid nodo: %d\n",getpid());
    /*leggo SO_TP_SIZE-1 transazioni dalla coda*/
    /*Eseguo somma di tutti i reward*/
    /*Aggiungo transazione reward*/
    l_add_transaction(new_transaction(5,REWARD_SENDER,getpid(), block_reward,0),&transaction_pool);
    if(SO_TP_SIZE<l_length(transaction_pool)){
        printf("Transaction pool is full\n");
        return 0;
    }
   l_print(transaction_pool);
    /*coda di messaggi*/
    /* semop con id che punta al semaforo per poter notificare al padre 
    che il nodo ha creato la sua coda di messaggi */
    exit(EXIT_SUCCESS);
}