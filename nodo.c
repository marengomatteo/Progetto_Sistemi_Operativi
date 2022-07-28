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
#define SO_BLOCK_SIZE atoi(getenv("SO_BLOCK_SIZE")) 
#define SH_NODES_ID atoi(argv[1])
#define SH_SEM_ID atoi(argv[2])
#define NODE_ID atoi(argv[3])
#define MASTERBOOK_ID atoi(argv[4])
#define REWARD_SENDER -1 
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))

/*Semaforo per segnalare che i nodi sono pronti*/
#define ID_READY 0;

int sem_nodes_id;
int block_reward;
int r_time;
struct timespec timestamp;
node_struct *nodes;

struct sembuf sops;
struct msgbuf_trans {
    long mtype;      
    transaction* trans;    
} msg;

/*Create transaction pool list*/
list transaction_pool;
block transaction_block;

int main(int argc, char *argv[])
{    
    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(SH_SEM_ID, &sops, 1);

    printf("sono in nodo\n");
    /* Mi attacco alle memorie condivise */
    nodes = shmat(SH_NODES_ID, NULL, 0);
    TEST_ERROR;
    /*printf("\n mi sono connesso alla memoria condivisa con id: %d\n", SH_NODES_ID);
    stampaStatoMemoria(SH_NODES_ID);*/

    clock_gettime(CLOCK_REALTIME, &timestamp);
    TEST_ERROR;
    while (1)
    {
    
    /*Prelevo dalla coda SO_TP_SIZE-1 transazioni */
    while(l_length(transaction_pool)<= SO_TP_SIZE){
        /* ricevo mex */
        msgrcv(nodes[NODE_ID].id_mq, &msg, sizeof(msg), 0, 0);
        l_add_transaction(msg.trans,&transaction_pool);
    }
    if(l_length(transaction_pool)> SO_TP_SIZE){
        printf("Transaction pool is full\n");
        return 0;
    }

    while(l_length(transaction_block.transaction_array)<SO_BLOCK_SIZE-1)
    {
        l_add_transaction(transaction_pool,&transaction_block.transaction_array);
        transaction_pool=transaction_pool->next;
    }
    if(l_length(transaction_block.transaction_array)==SO_BLOCK_SIZE-1){
    /*Aggiungo transazione di reward*/
    l_add_transaction(new_transaction(timestamp.tv_nsec,REWARD_SENDER,getpid(), block_reward,0),&transaction_block.transaction_array);
    /*msgctl(nodes[NODE_ID].id_mq,0,IPC_RMID);
    TEST_ERROR;*/
    /* aggiungo id blocco */
    r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
    timestamp.tv_nsec=r_time;
    nanosleep(&timestamp, NULL);
    exit(EXIT_SUCCESS);
    }
    }
}
