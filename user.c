#define _GNU_SOURCE
#include "user.h"

/* Costanti che servono per i parametri passati come argomento */
#define SH_USERS_ID atoi(argv[0])
#define SH_NODES_ID atoi(argv[1])
#define MASTERBOOK_ID atoi(argv[2])
#define USER_ID atoi(argv[3])
#define SEM_ID atoi(argv[4])
#define MASTERBOOK_INFO_ID atoi(argv[5])
#define SEM_MASTERBOOK_INFO_ID atoi(argv[6]) /* semaforo usato quando accediamo alle info del amsterbook*/
#define SEM_USERS_ID atoi(argv[7]) /* semaforo usato per accedere alle informazioni dello user*/

#define SO_USERS_NUM atoi(getenv("SO_USERS_NUM"))
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))
#define SO_REWARD atoi(getenv("SO_REWARD"))
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))
#define SO_RETRY atoi(getenv("SO_RETRY"))

/*-----------------------------------
|   Dichiarazione variabili globali |
-----------------------------------*/
node_struct* nodes;
user_struct* users;
block* masterbook;
masterbook_struct* shd_masterbook_info;
message msg;
rejected_message rejected_msg;

struct timespec timestamp;
struct sembuf sops;
struct sembuf sop_p; /* prende la risorsa*/
struct sembuf sop_r; /* rilascia la risorsa */

int curr_balance;   
int last_block;
/* PARTE DA 24 
int id_queue_message_rejected;*/

/*----------------------------
|     Variabili per argv      |
-----------------------------*/
int user_id;
int retry;
int sem_users_id;

void sig_transaction(int signum){
    switch(signum){
            case SIGUSR1:
                /* printf("CREO TRANSACTION\n"); */
                send_transaction();
                break;
            default:
            break;
    }
}

int main(int argc, char *argv[])
{
    /*----------------------------
    |   Dichiarazione variabili  |
    -----------------------------*/

    user_id=USER_ID;

    /* Variabile per ripetere la transazione in caso di fallimento*/
    retry = SO_RETRY;
    sem_users_id=SEM_USERS_ID;


   /* PARTE DA 24
   rejected_message=malloc(sizeof(rejected_message));*/
    /* Current Balance */
    
    curr_balance = SO_BUDGET_INIT;

    /*Mi aggancio alla memoria condivisa dei nodi*/
    nodes = shmat(SH_NODES_ID, NULL, 0);
    
    /*Mi aggancio alla memoria condivisa degli utenti*/
    users = shmat(SH_USERS_ID, NULL, 0);

    /*Mi aggancio alla memoria condivisa del masterbook*/
    masterbook = shmat(MASTERBOOK_ID, NULL, 0);

    /* Mi attacco alla memoria condivisa delle info del masterbook */
    shd_masterbook_info = shmat(MASTERBOOK_INFO_ID, NULL, 0);
   
    /* PARTE DA 24
     Coda di messaggi per transazioni rifiutate 
    id_queue_message_rejected = msgget(ID_QUEUE_MESSAGE_REJECTED, IPC_CREAT | 0600);*/

    /* Riservo la risorsa*/
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;

    if (signal(SIGUSR1, sig_transaction)==SIG_ERR) {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }

    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    if(semop(SEM_ID, &sops, 1) == -1){
        perror("semaphore broke");
        exit(EXIT_FAILURE);
    }

    #if DEBUG == 1
        printf("Finito utenti e nodi in  user\n");
    #endif

    while (1)
    {   

        if(semop(SEM_MASTERBOOK_INFO_ID, &sop_p, 1) == -1){
            perror("errore nel semaforo masterbook preso\n");
            return -1;
        }
       
        last_block = shd_masterbook_info->last_block_id;
      
        if(semop(SEM_MASTERBOOK_INFO_ID, &sop_r, 1) == -1){
            perror("errore nel semaforo masterbook rilascio\n");
            return -1;
        }

        update_budget();

        send_transaction();
    }
}

void send_transaction(){
    int index_rnode, i;
    int r_time;
    if(curr_balance >= 2){
        /* printf("[USER %d] invia transaction\n",getpid());
        indice random per prendere un nodo a cui inviare la transazione da processare*/
        srand(clock());

        index_rnode = rand() % shd_masterbook_info->num_nodes;

        msg.mtype = nodes[index_rnode].pid;
        msg.trans = build_transaction();

        #if DEBUG == 1
            printf("\ntransaction user pid: %d:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n", getpid(),msg.trans.timestamp_sec, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);
        #endif

        for(i=0; i<retry; i++){
            if(msgsnd(nodes[index_rnode].id_mq,&msg,sizeof(message),0) < 0) {
                printf("message send fallita\n");
            }
            else{

                /*printf("user %d fine\n", getpid());
                printf("\ntransaction user pid: %d:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n", getpid(),msg.trans.timestamp_sec, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);*/
             
                curr_balance = curr_balance - msg.trans.amount - msg.trans.reward;
                /* printf("[USER %d] curr balance %d\n", getpid(), curr_balance);*/
                /* prendo il semaforo per aggiornare il bilancio*/
                if(semop(sem_users_id, &sop_p, 1) == -1){
                    perror("errore nel semaforo preso per bilancio user\n");
                }
        
                users[user_id].budget = curr_balance;
            
                /* rilascio il semaforo*/
                if(semop(sem_users_id, &sop_r, 1) == -1){
                    perror("errore nel semaforo rilasciato per bilancio user\n");
                }
                r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
                timestamp.tv_sec = 0;
                timestamp.tv_nsec=r_time;
                nanosleep(&timestamp, NULL);
                break;
            }
        } 
        if(i==retry){
            if(semop(sem_users_id, &sop_p, 1) == -1){
                perror("errore nel semaforo preso per status\n");
            }

            users[user_id].status = 0;

            if(semop(sem_users_id, &sop_r, 1) == -1){
                perror("errore nel semaforo preso per status\n");
            }
            exit(EXIT_FAILURE);
        } 
    }
}

void update_budget(){
    int index_transaction;
    while(users[user_id].last_block_read < last_block){
        #if DEBUG == 1
            printf("[USER %d] current budget prima: %d\n", getpid(), curr_balance);
        #endif

        
        for(index_transaction = 0; index_transaction < SO_BLOCK_SIZE; index_transaction++){
            if(semop(shd_masterbook_info->sem_masterbook, &sop_p, 1) == -1) {
                printf("errore semaforo user\n");
            }
            if(masterbook[users[user_id].last_block_read].transaction_array[index_transaction].receiver == getpid()){
                curr_balance += masterbook[users[user_id].last_block_read].transaction_array[index_transaction].amount;
               /* printf("[USER %d] curr balance AGGIORNATO %d di %d\n", getpid(), curr_balance, masterbook[users[user_id].last_block_read].transaction_array[index_transaction].amount);*/
            }

            /* Rilascio il semaforo */
            if (semop(shd_masterbook_info->sem_masterbook, &sop_r, 1) == -1) {
                printf("errore nel rilascio del semaforo\n");
            }
        }

        /*  PARTE DA 24
        while(msgrcv(id_queue_message_rejected, &rejected_msg, sizeof(rejected_message), getpid(), IPC_NOWAIT) > 0){
            curr_balance = curr_balance + rejected_msg.amount;
        }
        */

        /* prendo il semaforo per aggiornare il bilancio*/
        if(semop(sem_users_id, &sop_p, 1) == -1){
            perror("errore nel semaforo preso per bilancio\n");
        }
    
        users[user_id].budget = curr_balance;
        users[user_id].last_block_read++;

        /* rilascio il semaforo*/
        if(semop(sem_users_id, &sop_r, 1) == -1){
            perror("errore nel semaforo rilasciato per bilancio\n");
        }

        #if DEBUG == 1
            printf("[USER %d] current budget dopo: %d\n", getpid(), curr_balance);
        #endif
    }
}

transaction build_transaction(){
        
        transaction t;
        int index_ruser;
        int calculate_reward;
        int r_number;

        srand(clock());
        do{
            index_ruser= rand() % SO_USERS_NUM;
        }while (index_ruser == user_id);
        
        /* calcolo di quanti soldi devo inviare all'altro user*/
        r_number = (rand() % (curr_balance + 1 - 2)) + 2;

        /* calcolo reward */
        calculate_reward = floor(r_number/100*SO_REWARD) ;
        calculate_reward= (calculate_reward > 1) ? calculate_reward : 1;
        clock_gettime(CLOCK_REALTIME, &timestamp);

        /* Creo la transazione da spedire */
        t.timestamp_nsec = timestamp.tv_nsec;
        t.timestamp_sec = timestamp.tv_sec;
        t.sender = getpid();
        t.receiver = users[index_ruser].pid;
        t.reward = calculate_reward;
        t.amount = r_number-calculate_reward;

        #if DEBUG==1
        printf("\ntransaction user pid: %d:{timestamp: nsec: %ld sec: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n", getpid(),t.timestamp_nsec, t.timestamp_sec, t.sender, t.receiver, t.amount, t.reward);
        #endif
        
        return t;
}
