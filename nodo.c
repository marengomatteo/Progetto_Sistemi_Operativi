#define _GNU_SOURCE

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
#define SH_NODES_ID atoi(argv[1])
#define SH_SEM_ID atoi(argv[2])
#define NODE_ID atoi(argv[3])
#define MASTERBOOK_ID atoi(argv[4])
#define REWARD_SENDER -1 
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))

#define SO_TP_SIZE atoi(getenv("SO_TP_SIZE"))

/*Semaforo per segnalare che i nodi sono pronti*/
#define ID_READY 0;

int sem_nodes_id;
int block_reward=0;
int r_time;
struct timespec timestamp;
node_struct *nodes;
block* masterbook;
struct sembuf sops;

/*Create transaction pool list*/
list transaction_pool;
block transaction_block;
int index_block = 0;
struct Message {
    long mtype;      
    transaction trans;    
} msg;

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

    /* Mi attacco alle memorie condivise */
    masterbook = shmat(MASTERBOOK_ID, NULL, 0);
    TEST_ERROR;

    /*printf("\n mi sono connesso alla memoria condivisa con id: %d\n", SH_NODES_ID);
    stampaStatoMemoria(SH_NODES_ID);*/

    clock_gettime(CLOCK_REALTIME, &timestamp);
    TEST_ERROR;
    #if DEBUG == 1
        printf("prima del while 1\n");
    #endif
    while (1){
        /*Prelevo dalla coda SO_TP_SIZE-1 transazioni */
        while(l_length(transaction_pool) < SO_TP_SIZE && msgrcv(nodes[NODE_ID].id_mq, &msg, sizeof(struct Message), nodes[NODE_ID].pid,IPC_NOWAIT)>0){
            /* receiving message */
            #if DEBUG == 1
                printf("ricevo la transazione\n");
            #endif
            #if DEBUG == 1
                printf("\ntransaction nodo:{\n\ttimestamp: %ld,\n\tsender: %d,\n\treceiver: %d,\n\tamount: %d,\n\treward: %d\n}\n", msg.trans.timestamp, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);
            #endif
            l_add_transaction(msg.trans,&transaction_pool);
                
        }

        if(l_length(transaction_pool) > SO_TP_SIZE){
            return 0;
        }

        while(l_length(transaction_pool)>0 && index_block<SO_BLOCK_SIZE-1) {
            #if DEBUG == 1
                printf("aggiungo transazione dalla transaction pool al blocco\n");
            #endif
            transaction_block.transaction_array[index_block] = (*transaction_pool).transaction;
            index_block++;
            block_reward+= (*transaction_pool).transaction.reward;
            transaction_pool = transaction_pool->next;
            #if DEBUG == 1
                printf("lista transazioni nel blocco\n");
                l_print(transaction_block.transaction_array);
            #endif
        }

        if(index_block==SO_BLOCK_SIZE-1){
            /*Add reward transaction*/
            printf("block reward: %d\n", block_reward);
            transaction_block.transaction_array[index_block] = new_transaction(timestamp.tv_nsec,REWARD_SENDER,getpid(), block_reward,0);
            block_reward = 0;
            r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
            timestamp.tv_nsec=r_time;
            nanosleep(&timestamp, NULL);
        }


    

    }
}

  
 /* while(l_length(transaction_pool)>=0){
           transaction_print(&transaction_pool->transaction);
       }
       */
