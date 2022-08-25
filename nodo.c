#define _GNU_SOURCE

#include "nodo.h"

/* Costanti che servono per i parametri passati come argomento */

#define SH_NODES_ID atoi(argv[0])
#define NODE_ID atoi(argv[1])
#define MASTERBOOK_ID atoi(argv[2])
#define MASTERBOOK_INFO_ID atoi(argv[3])
#define SEM_MASTERBOOK_INFO_ID atoi(argv[4])
#define SEM_NODES_USERS atoi(argv[5])
#define SEM_NODES atoi(argv[6])

#define REWARD_SENDER -1 

#define SO_MIN_TRANS_PROC_NSEC atoi(getenv("SO_MIN_TRANS_PROC_NSEC"))
#define SO_MAX_TRANS_PROC_NSEC atoi(getenv("SO_MAX_TRANS_PROC_NSEC"))
#define SO_TP_SIZE atoi(getenv("SO_TP_SIZE"))
#define SO_HOPS atoi(getenv("SO_HOPS"))


/*-----------------------------------
|   Dichiarazione variabili globali |
-----------------------------------*/
int block_reward = 0;
node_struct *nodes;
masterbook_struct* shd_masterbook_info;
block* masterbook;
list transaction_pool;
/* PARTE DA 24
rejected_message rejected_msg;*/
message msg;
message_f msg_friend;
message_id_f msg_id_f;
int* friends;
/* ---------------------------
|   Dichiarazione semafori   |
----------------------------*/
struct sembuf sops;
struct sembuf sop_p; /* prende la risorsa*/
struct sembuf sop_r; /* rilascia la risorsa */

/* Variabili globali per il blocco */ 
block transaction_block;
int index_block = 0;

int main(int argc, char *argv[])
{    
    /* ---------------------------
    |   Dichiarazione variabili  |
    ----------------------------*/
    int r_time, r_friend;
   /* PARTE DA 24 int id_queue_message_rejected;*/
    int id_queue_friends,id_queue_pid_friends;
    struct timespec timestamp;
    int j,i;
    int num_friends = SO_NUM_FRIENDS;


    friends = malloc(sizeof(int)*num_friends);
    
    /* Mi attacco alle memorie condivise */
    nodes = shmat(SH_NODES_ID, NULL, 0);

    /* Mi attacco alle memorie condivise */
    masterbook = shmat(MASTERBOOK_ID, NULL, 0);

    /* Mi attacco alle memorie condivisa del masterbook */
    shd_masterbook_info = shmat(MASTERBOOK_INFO_ID, NULL, 0);

    /* PARTE DA 24
    Coda di messaggi per le transazoni rifiutate 
    id_queue_message_rejected = msgget(ID_QUEUE_MESSAGE_REJECTED,IPC_CREAT | 0600);*/

    /*Creo coda di messaggi amici*/
    id_queue_friends = msgget(ID_QUEUE_FRIENDS,IPC_CREAT | 0600);

    /*Creo coda di messaggi pid amici*/
    id_queue_pid_friends = msgget(ID_QUEUE_FRIENDS_PID,IPC_CREAT | 0600);

    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_op = 0;
    sops.sem_num = 0;
    semop(SEM_NODES_USERS, &sops, 1);


    for(i=0; i<num_friends;i++){
        /* printf("nodo creato %d\n", getpid());*/ 
        if(msgrcv(id_queue_pid_friends, &msg_id_f, sizeof(message_id_f), getpid(),0)>0){
            friends[i] = msg_id_f.friend;
        }else{
            printf("errore ricezione messaggio amico\n");
            exit(EXIT_FAILURE);
        }
    }
    
   
    /* Riservo la risorsa*/
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;

    while (1){

        /* controllo se sono arrivati altri amici*/
        if(msgrcv(id_queue_pid_friends, &msg_id_f, sizeof(message_id_f), getpid(),IPC_NOWAIT) > 0){

                num_friends++;
                
                friends = (int*)realloc(friends,num_friends * sizeof(int));

                #if DEBUG == 1
                    printf("[NODO %d] amico %d\n", getpid(), friends[num_friends-1]); */
                #endif

                friends[num_friends-1] = msg_id_f.friend;

                #if DEBUG == 1
                    for(i = 0; i < num_friends; i++){
                        printf("[NODO %d] amico %d: %d\n", getpid(), i, friends[i]);
                    }
                #endif
        }
     
        /*Prelevo dalla coda e inserisco massimo SO_TP_SIZE-1 transazioni nella transaction pool*/
        while(msgrcv(nodes[NODE_ID].id_mq, &msg, sizeof(message), getpid(),IPC_NOWAIT)>0){
            #if DEBUG == 1
                printf("ricevo mex nodo %d\n", getpid());
            
                printf("[NODO %d] ricevo transazione\n", getpid());
            
                printf("\ntransaction nodo: %d:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n", getpid(),msg.trans.timestamp, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);
            #endif
           
            if(nodes[NODE_ID].tp_size == SO_TP_SIZE){

                /* PARTE DA 24 */
                /* messaggio di transazione rifiutata, rispedito allo user che aggiornerà il suo bilancio 
                rejected_msg.mtype = msg.trans.sender;
                rejected_msg.amount = msg.trans.amount + msg.trans.reward;

                if(msgsnd(id_queue_message_rejected,&rejected_msg,sizeof(rejected_message),0) < 0){
                    printf("errore nella message send");
                    return -1;
                }*/

                /* PARTA DA 30 */
                /*scelgo amico random*/
                srand(clock());
                r_friend = rand() % num_friends;
                msg_friend.mtype = friends[r_friend];
                msg_friend.trans = msg.trans;
                msg_friend.hops = 0;

                #if DEBUG == 1
                    printf("[NODO %d] to: %d timestamp: %ld\n", getpid(), friends[r_friend], msg_friend.trans.timestamp);
                #endif

                if(msgsnd(id_queue_friends,&msg_friend,sizeof(message_f),0) < 0){
                    perror("errore nella message send ad un amico per tp piena\n");
                    return -1;
                }
                
                #if DEBUG == 1
                    printf("nodo %d invio a master\n", getpid());
                #endif

            }
            else{

                #if DEBUG == 1
                    printf("[NODO %d] riceve transaction\n", getpid());
                #endif

                /* aggiungo transazione alla transaction pool */
                l_add_transaction(msg.trans,&transaction_pool);

                if(semop(SEM_NODES, &sop_p, 1) == -1){
                    perror("errore nel semaforo dei nodi preso\n");
                    return -1;
                }
       
                /* aumento la transaction pool size nella memoria condivisa */
                nodes[NODE_ID].tp_size++;
      
                if(semop(SEM_NODES, &sop_r, 1) == -1){
                    perror("errore nel semaforo dei nodi rilascio\n");
                    return -1;
                }
            }

        }

        /* ricevo messaggi da amici */
        while(msgrcv(id_queue_friends, &msg_friend, sizeof(message_f), getpid(),IPC_NOWAIT)>0){

            if(nodes[NODE_ID].tp_size == SO_TP_SIZE){

                if(msg_friend.hops < SO_HOPS){
                    srand(clock());
                    r_friend = rand() % num_friends;
                    msg_friend.mtype = friends[r_friend];
                    msg_friend.hops = msg_friend.hops++;
                    /*printf("[LINEA 194 NODO %d] msg_friend type %ld: { hops: %d, sender: %d, receiver: %d, timestamp %ld }\n", getpid(),msg_friend.mtype, msg_friend.hops, msg_friend.trans.sender, msg_friend.trans.receiver, msg_friend.trans.timestamp);*/

                    if(msgsnd(id_queue_friends,&msg_friend,sizeof(message_f),0) < 0){
                        perror("errore nella message send ad un amico mex ricevuto da amico\n");
                        return -1;
                    }
                }
                else {
                    msg_friend.mtype = getppid();
                    /*printf("[LINEA 204 NODO %d] msg_friend type %ld: { hops: %d, sender: %d, receiver: %d, timestamp %ld }\n", getpid(),msg_friend.mtype, msg_friend.hops, msg_friend.trans.sender, msg_friend.trans.receiver, msg_friend.trans.timestamp);*/
                    if(msgsnd(id_queue_friends,&msg_friend,sizeof(message_f),0) < 0){
                        printf("errore nella message send a master");
                        return -1;
                    }
               }
            }
            else{
                l_add_transaction(msg_friend.trans,&transaction_pool);

                if(semop(SEM_NODES, &sop_p, 1) == -1){
                    perror("errore nel semaforo dei nodi preso\n");
                    return -1;
                }
       
                /* aumento la transaction pool size nella memoria condivisa */
                nodes[NODE_ID].tp_size++;
      
                if(semop(SEM_NODES, &sop_r, 1) == -1){
                    perror("errore nel semaforo dei nodi rilascio\n");
                    return -1;
                }

            }
        }
        /* Invio una transaction ad un amico */
        if(nodes[NODE_ID].tp_size > 0){
            /*scelgo amico random*/
            srand(clock());
            r_friend = rand() % num_friends;
            /* creo messaggio */
            msg_friend.mtype = friends[r_friend];
            msg_friend.trans = transaction_pool.head->transaction;
            msg_friend.hops = 0;

            if(msgsnd(id_queue_friends,&msg_friend,sizeof(message_f),0) < 0){
                    perror("errore nella message send ad un amico pescato a caso\n");
                    return -1;
            }

            /* scarto transazione dalla transaction pool*/
            transaction_pool.head = transaction_pool.head->next;
            if(transaction_pool.head == NULL) 
                transaction_pool.tail = NULL;

            if(semop(SEM_NODES, &sop_p, 1) == -1){
                perror("errore nel semaforo dei nodi preso\n");
                return -1;
            }
    
            nodes[NODE_ID].tp_size--;
    
            if(semop(SEM_NODES, &sop_r, 1) == -1){
                perror("errore nel semaforo dei nodi rilascio\n");
                return -1;
            }
        }

        while(nodes[NODE_ID].tp_size > 0){

            /* Creo un nuovo blocco*/
            build_block(NODE_ID, SEM_NODES);

            if(index_block == SO_BLOCK_SIZE-1){

                /*Creo la transazione di reward e la aggiungo */
                clock_gettime(CLOCK_REALTIME, &timestamp);
                transaction_block.transaction_array[index_block] = create_reward_transaction(timestamp.tv_nsec,REWARD_SENDER,getpid(), block_reward,0);
                block_reward = 0;
                index_block = 0;

                /* simulo attesa per processare il blocco */
                /*Devo usare clock() perchè succede tutto in nanosecondi e time() rileva i secondi*/
                srand(clock());

                r_time = (rand()%(SO_MAX_TRANS_PROC_NSEC+1-SO_MIN_TRANS_PROC_NSEC))+SO_MIN_TRANS_PROC_NSEC;
                timestamp.tv_sec = 0;
                timestamp.tv_nsec = r_time;
                nanosleep(&timestamp, NULL);

                if(semop(shd_masterbook_info->sem_masterbook, &sop_p,1) == -1){
                    perror("errore nel semaforo nodo\n");
                    return -1;
                }
                
                /* prendo un nuovo id blocco */
                transaction_block.id_block = new_id_block(SEM_MASTERBOOK_INFO_ID);

                if(transaction_block.id_block < SO_REGISTRY_SIZE){

                    /* aggiungo il blocco al masterbook */
                    masterbook[transaction_block.id_block] = transaction_block;

                    if(semop(SEM_NODES, &sop_p, 1) == -1){
                        perror("errore nel semaforo dei nodi preso\n");
                        exit(EXIT_FAILURE);
                    }
                
                    nodes[NODE_ID].budget+= transaction_block.transaction_array[SO_BLOCK_SIZE-1].amount;

                    if(semop(SEM_NODES, &sop_r, 1) == -1){
                        perror("errore nel semaforo dei nodi rilascio\n");
                        exit(EXIT_FAILURE);
                    }

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
        printf("errore semaforo in nodo\n");
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

void build_block(int node_id, int sem_nodes){

    while( nodes[node_id].tp_size > 0 && index_block < SO_BLOCK_SIZE-1) {

        /* inserisco nell'array delle transazioni del blocco la transazione processata */
        transaction_block.transaction_array[index_block] = transaction_pool.head->transaction;
        
        index_block++;

        /* reward del blocco servirà per la transazione di reward */
        block_reward += transaction_pool.head->transaction.reward;

        /* scarto la transazione dalla transaction pool */
        transaction_pool.head = transaction_pool.head->next;
        
        if(transaction_pool.head == NULL) 
            transaction_pool.tail = NULL;


        if(semop(sem_nodes, &sop_p, 1) == -1){
            perror("errore nel semaforo dei nodi preso\n");
            exit(EXIT_FAILURE);
        }
       
        /* diminuisco la size della transaction pool sulla memoria condivisa*/
        nodes[node_id].tp_size--;

        if(semop(sem_nodes, &sop_r, 1) == -1){
            perror("errore nel semaforo dei nodi rilascio\n");
            exit(EXIT_FAILURE);
        }
    }
}