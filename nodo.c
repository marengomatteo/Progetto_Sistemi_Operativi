#define _GNU_SOURCE

#include "nodo.h"

/* Costanti che servono per i parametri passati come argomento */

#define SH_NODES_ID atoi(argv[0])
#define SH_SEM_ID atoi(argv[1])
#define NODE_ID atoi(argv[2])
#define MASTERBOOK_ID atoi(argv[3])
#define MASTERBOOK_INFO_ID atoi(argv[4])
#define SEM_MASTERBOOK_INFO_ID atoi(argv[5])

#define REWARD_SENDER -1 
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))

#define SO_TP_SIZE atoi(getenv("SO_TP_SIZE"))

/*Semaforo per segnalare che i nodi sono pronti*/
#define ID_READY 0;

int sem_nodes_id;
int block_reward = 0;
int r_time;
struct timespec timestamp;
node_struct *nodes;
masterbook_struct* shd_masterbook_info;
block* masterbook;

/* semafori */
struct sembuf sops;
struct sembuf sop_p; /* prende la risorsa*/
struct sembuf sop_r; /* rilascia la risorsa */

int id_blocco = 0;
/*Create transaction pool list*/
list transaction_pool;
block transaction_block;
int index_block = 0;
struct Message {
    long mtype;      
    transaction trans;    
} msg;

int nuovo_id_blocco(int sem_id){

    struct sembuf sop_p; /* prende la risorsa*/
    struct sembuf sop_r; /* rilascia la risorsa */
    int block_id;

    /* Riservo la risorsa*/
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;

    if(semop(sem_id, &sop_p, 1) == -1) {
        printf("errore semaforo\n");
        return -1;
    }

    block_id = shd_masterbook_info->last_block_id;
    shd_masterbook_info->last_block_id++;

    /* Rilascio il semaforo */
    if (semop(sem_id, &sop_r, 1) == -1) {
        printf("errore nel rilascio del semaforo\n");
        return -1;
    }

    return block_id;
}

int main(int argc, char *argv[])
{    

    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(SH_SEM_ID, &sops, 1);

    /* Mi attacco alle memorie condivise */
    nodes = shmat(SH_NODES_ID, NULL, 0);
    nodes[NODE_ID].budget=0;
    /* Mi attacco alle memorie condivise */
    masterbook = shmat(MASTERBOOK_ID, NULL, 0);

    /* Mi attacco alle memorie condivisa del masterbook */
    shd_masterbook_info = shmat(MASTERBOOK_INFO_ID, NULL, 0);

    clock_gettime(CLOCK_REALTIME, &timestamp);
   
    /* Riservo la risorsa*/
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;

    while (1){
       
        /*Prelevo dalla coda SO_TP_SIZE-1 transazioni */
        while(l_length(transaction_pool) < SO_TP_SIZE && msgrcv(nodes[NODE_ID].id_mq, &msg, sizeof(struct Message), nodes[NODE_ID].pid,IPC_NOWAIT)>0){
              
            /*#if DEBUG == 1
                printf("ricevo la transazione\n");
                printf("\ntransaction nodo:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n\n", msg.trans.timestamp, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);
            #endif*/

            l_add_transaction(msg.trans,&transaction_pool);
              
        }

        if(l_length(transaction_pool) > SO_TP_SIZE){
            printf("transaction pool piena");
            return -1;
        }

        while(l_length(transaction_pool)>0 && index_block < SO_BLOCK_SIZE-1) {
            /*#if DEBUG == 1
                printf("aggiungo transazione dalla transaction pool al blocco\n");
            #endif*/

            transaction_block.transaction_array[index_block] = (*transaction_pool).transaction;
            index_block++;

            block_reward+= (*transaction_pool).transaction.reward;
            transaction_pool = (*transaction_pool).next;
            /*#if DEBUG == 1
                printf("lista transazioni nel blocco %d\n", index_block);
            #endif*/
        }

        if(index_block == SO_BLOCK_SIZE-1){
            /*Add reward transaction */
            transaction_block.transaction_array[index_block] = new_transaction(timestamp.tv_nsec,REWARD_SENDER,getpid(), block_reward,0);
            block_reward = 0;
            index_block = 0;
            r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
            timestamp.tv_nsec = r_time;
            nanosleep(&timestamp, NULL);

            if(semop(shd_masterbook_info->sem_masterbook, &sop_p,1) == -1){
                perror("errore nel semaforo\n");
                return -1;
            }
            transaction_block.id_block = nuovo_id_blocco(SEM_MASTERBOOK_INFO_ID);
            printf("id blocco %d\n",transaction_block.id_block);
            if(transaction_block.id_block <= SO_REGISTRY_SIZE-1){
                masterbook[transaction_block.id_block] = transaction_block;
                nodes[NODE_ID].budget+= transaction_block.transaction_array[SO_BLOCK_SIZE-1].amount;
                a_print(masterbook[transaction_block.id_block]);
            }
            if(semop(shd_masterbook_info->sem_masterbook, &sop_r,1) == -1){
                perror("errore nel rilascio del semaforo\n");
                return -1;
            }
        }
        
    }

}
