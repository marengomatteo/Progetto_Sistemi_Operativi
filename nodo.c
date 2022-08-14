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
int id_queue_message_rejected;
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

struct rejected_message{
    long mtype;
    int amount;
    pid_t receiver;
} rejected_msg;

int main(int argc, char *argv[])
{    

    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(SH_SEM_ID, &sops, 1);
    printf("Finito utenti e nodi in  nodo\n");

    /* Mi attacco alle memorie condivise */
    nodes = shmat(SH_NODES_ID, NULL, 0);

    /* Mi attacco alle memorie condivise */
    masterbook = shmat(MASTERBOOK_ID, NULL, 0);

    /* Mi attacco alle memorie condivisa del masterbook */
    shd_masterbook_info = shmat(MASTERBOOK_INFO_ID, NULL, 0);

    /* Coda di messaggi per le transazoni rifiutate */
    id_queue_message_rejected = msgget(ID_QUEUE_MESSAGE_REJECTED,IPC_CREAT | 0600);

    clock_gettime(CLOCK_REALTIME, &timestamp);
   
    /* Riservo la risorsa*/
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;

    while (1){
       sleep(2);
        /*Prelevo dalla coda SO_TP_SIZE-1 transazioni */
        while(msgrcv(nodes[NODE_ID].id_mq, &msg, sizeof(struct Message), nodes[NODE_ID].pid,IPC_NOWAIT)>0){
            /*#if DEBUG == 1
                printf("ricevo la transazione\n");
                printf("\ntransaction nodo:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n\n", msg.trans.timestamp, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);
            #endif*/
            if(l_length(transaction_pool) >= SO_TP_SIZE){
                
                rejected_msg.mtype = msg.trans.sender;
                rejected_msg.amount = msg.trans.amount + msg.trans.reward;
                rejected_msg.receiver = msg.trans.sender;

                if(msgsnd(id_queue_message_rejected,&rejected_msg,sizeof(struct rejected_message),0) < 0){
                    printf("errore nella message send");
                    return -1;
                }
            }
            else{
                l_add_transaction(msg.trans,&transaction_pool);
            }

              
        }

        while(l_length(transaction_pool)>0 && index_block < SO_BLOCK_SIZE-1) {

            transaction_block.transaction_array[index_block] = transaction_pool.head->transaction;
            index_block++;

            block_reward += transaction_pool.head->transaction.reward;
            transaction_pool.head = transaction_pool.head->next;
            if(transaction_pool.head==NULL)transaction_pool.tail=NULL;
           
        }
           

        if(index_block == SO_BLOCK_SIZE-1){
            /*Add reward transaction */
            transaction_block.transaction_array[index_block] = reward_transaction(timestamp.tv_nsec,REWARD_SENDER,getpid(), block_reward,0);
            block_reward = 0;
            index_block = 0;
            r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
            timestamp.tv_nsec = r_time;
            nanosleep(&timestamp, NULL);

            if(semop(shd_masterbook_info->sem_masterbook, &sop_p,1) == -1){
                perror("errore nel semaforo\n");
                return -1;
            }
            
            /* prendo un nuovo id blocco*/
            transaction_block.id_block = nuovo_id_blocco(SEM_MASTERBOOK_INFO_ID);

            if(transaction_block.id_block <= SO_REGISTRY_SIZE-1){
                masterbook[transaction_block.id_block] = transaction_block;
                nodes[NODE_ID].budget+= transaction_block.transaction_array[SO_BLOCK_SIZE-1].amount;
            }
            if(semop(shd_masterbook_info->sem_masterbook, &sop_r,1) == -1){
                perror("errore nel rilascio del semaforo\n");
                return -1;
            }
        }
        
    }

}

transaction reward_transaction(long timestamp, int sender, int receiver, int amount, int reward){
    transaction* d = malloc(sizeof(transaction));
    d->timestamp = timestamp;
    d->sender = sender;
    d->receiver = receiver;
    d->amount = amount;
    d->reward = reward;
    return *d;
}

void l_add_transaction(transaction d, list* l){
  node *n = (node*)malloc(sizeof(node));
  n->transaction = d;
  n->next = NULL;

    if(l->head==NULL){
        l->head=n;
    }else{
        l->tail->next = n;
    }
    l->tail=n;
}

int l_length(list l){
  int length;
  node* tmp=l.head;
  length = 0;
  while(tmp != NULL){
    length++;
    tmp = tmp->next;
  }
printf("lunghezza: %d", length);
  return length;
}

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
