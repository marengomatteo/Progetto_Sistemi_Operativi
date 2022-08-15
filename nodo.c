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


/*-----------------------------------
|   Dichiarazione variabili globali |
-----------------------------------*/
int block_reward = 0;
node_struct *nodes;
masterbook_struct* shd_masterbook_info;
block* masterbook;
list transaction_pool;
rejected_message rejected_msg;
message msg;

/* Variabili globali per il blocco */ 
block transaction_block;
int index_block = 0;

int main(int argc, char *argv[])
{    
    /* ---------------------------
    |   Dichiarazione variabili  |
    ----------------------------*/
    int r_time;
    int id_queue_message_rejected;
    struct timespec timestamp;

    /* ---------------------------
    |   Dichiarazione semafori   |
    ----------------------------*/
    struct sembuf sops;
    struct sembuf sop_p; /* prende la risorsa*/
    struct sembuf sop_r; /* rilascia la risorsa */

    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(SH_SEM_ID, &sops, 1);

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

        /*Prelevo dalla coda SO_TP_SIZE-1 transazioni */
        while(msgrcv(nodes[NODE_ID].id_mq, &msg, sizeof(message), nodes[NODE_ID].pid,IPC_NOWAIT)>0){

            if(l_length(transaction_pool) >= SO_TP_SIZE){
                
                /* messaggio di transazione rifiutata, rispedito allo user che aggiornerà il suo bilancio */
                rejected_msg.mtype = msg.trans.sender;
                rejected_msg.amount = msg.trans.amount + msg.trans.reward;
                rejected_msg.receiver = msg.trans.sender;

                if(msgsnd(id_queue_message_rejected,&rejected_msg,sizeof(rejected_message),0) < 0){
                    printf("errore nella message send");
                    return -1;
                }
            }
            else{
                /* aggiungo transazione alla transaction pool */
                l_add_transaction(msg.trans,&transaction_pool);

                /* aumento la transaction pool size nella memoria condivisa */
                nodes[NODE_ID].tp_size++;
            }

        }

        /* Creo un nuovo blocco*/
        build_block(NODE_ID);
                 

        if(index_block == SO_BLOCK_SIZE-1){
            /*Creo la transazione di reward e la aggiungo */
            transaction_block.transaction_array[index_block] = create_reward_transaction(timestamp.tv_nsec,REWARD_SENDER,getpid(), block_reward,0);
            block_reward = 0;
            index_block = 0;

            /* simulo attesa per processare il blocco */
            r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
            timestamp.tv_nsec = r_time;
            nanosleep(&timestamp, NULL);

            if(semop(shd_masterbook_info->sem_masterbook, &sop_p,1) == -1){
                perror("errore nel semaforo\n");
                return -1;
            }
            
            /* prendo un nuovo id blocco */
            transaction_block.id_block = new_id_block(SEM_MASTERBOOK_INFO_ID);

            if(transaction_block.id_block <= SO_REGISTRY_SIZE-1){
                /* aggiungo il blocco al masterbook */
                masterbook[transaction_block.id_block] = transaction_block;
                nodes[NODE_ID].budget+= transaction_block.transaction_array[SO_BLOCK_SIZE-1].amount;
                shd_masterbook_info->num_block++;
            }
            else {
                /* notifico al master che il masterbook è saturo */
                kill(getppid(), SIGUSR1);
            }

            if(semop(shd_masterbook_info->sem_masterbook, &sop_r,1) == -1){
                perror("errore nel rilascio del semaforo\n");
                return -1;
            }
        }
        
    }

}

transaction create_reward_transaction(long timestamp, int sender, int receiver, int amount, int reward){
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
  return length;
}

int new_id_block(int sem_id){

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

void build_block(int node_id){

    while(l_length(transaction_pool)>0 && index_block < SO_BLOCK_SIZE-1) {
        /* inserisco nell'array delle transazioni del blocco la transazione processata */
        transaction_block.transaction_array[index_block] = transaction_pool.head->transaction;
        
        index_block++;

        /* reward del blocco servirà per la transazione di reward */
        block_reward += transaction_pool.head->transaction.reward;

        transaction_pool.head = transaction_pool.head->next;
        
        if(transaction_pool.head == NULL) 
            transaction_pool.tail = NULL;

        /* diminuisco la size della transaction pool sulla memoria condivisa*/
        nodes[node_id].tp_size--;
    }
}