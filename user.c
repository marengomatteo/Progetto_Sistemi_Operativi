#define _GNU_SOURCE
#include "user.h"
#include <math.h>

/* Costanti che servono per i parametri passati come argomento */
#define SH_USERS_ID atoi(argv[0])
#define SH_NODES_ID atoi(argv[1])
#define MASTERBOOK_ID atoi(argv[2])
#define USER_ID atoi(argv[3])
#define SEM_ID atoi(argv[4])
#define MASTERBOOK_INFO_ID atoi(argv[5])
#define SEM_MASTERBOOK_INFO_ID atoi(argv[6]) /* semaforo usato quando accediamo alle info del amsterbook*/
#define SEM_USERS_ID atoi(argv[7]) /* semaforo usato per accedere alle informazioni dello user*/

#define SO_NODES_NUM atoi(getenv("SO_NODES_NUM"))
#define SO_USERS_NUM atoi(getenv("SO_USERS_NUM"))
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))
#define SO_REWARD atoi(getenv("SO_REWARD"))
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))
#define SO_RETRY atoi(getenv("SO_RETRY"))

node_struct* nodes;
user_struct* users;
masterbook_struct* shd_masterbook_info;
block* masterbook;
struct timespec timestamp;
struct sembuf sops;
struct sembuf sop_p; /* prende la risorsa*/
struct sembuf sop_r; /* rilascia la risorsa */

int block_id;
int index_rnode;
int index_ruser;
int curr_balance;   
int r_number;
int retry;
int calculate_reward;
int r_time;
int last_block;
int id_queue_message_rejected;

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
    int index_transaction;

    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    if(semop(SEM_ID, &sops, 1) == -1){
        perror("semaphore broke");
        exit(EXIT_FAILURE);
    }
        printf("Finito utenti e nodi in  user\n");


    /* Current Balance */
    curr_balance = SO_BUDGET_INIT;

    /*Mi aggancio alla memoria condivisa dei nodi*/
    nodes = shmat(SH_NODES_ID, NULL, 0);
    
    /*Mi aggancio alla memoria condivisa degli utenti*/
    users = shmat(SH_USERS_ID, NULL, 0);

    /*Mi aggancio alla memoria condivisa del masterbook*/
    masterbook = shmat(MASTERBOOK_ID, NULL, 0);

    /* Mi attacco alle memorie condivisa del masterbook */
    shd_masterbook_info = shmat(MASTERBOOK_INFO_ID, NULL, 0);
   
    /* Coda di messaggi per transazioni rifiutate */
    id_queue_message_rejected = msgget(ID_QUEUE_MESSAGE_REJECTED, IPC_CREAT | 0600);

    /* Riservo la risorsa*/
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;


    srand(getpid());

    while (1)
    {   
        if(semop(SEM_MASTERBOOK_INFO_ID, &sop_p, 1) == -1){
            perror("errore nel semaforo\n");
            return -1;
        }
       
        last_block = shd_masterbook_info->last_block_id;
      
        if(semop(SEM_MASTERBOOK_INFO_ID, &sop_r, 1) == -1){
            perror("errore nel semaforo\n");
            return -1;
        }

        while(users[USER_ID].last_block_read < last_block){
            if(semop(shd_masterbook_info->sem_masterbook, &sop_p, 1) == -1) {
                printf("errore semaforo\n");
                return -1;
            }
            /*printf("riservo semaforo per lettura user: %d\n",getpid());
            printf("leggo blocco %d\n",users[USER_ID].last_block_read );*/
            for(index_transaction = 0; index_transaction < SO_BLOCK_SIZE; index_transaction++){
                if(masterbook[users[USER_ID].last_block_read].transaction_array[index_transaction].receiver == getpid()){
                    curr_balance += masterbook[users[USER_ID].last_block_read].transaction_array[index_transaction].amount;
                }
            }


            /* Rilascio il semaforo */
            /*printf("rilascio semaforo per lettura user: %d current_balance: %d\n",getpid(),curr_balance);*/
            if (semop(shd_masterbook_info->sem_masterbook, &sop_r, 1) == -1) {
                printf("errore nel rilascio del semaforo\n");
                return -1;
            }

            while(msgrcv(id_queue_message_rejected, &rejected_msg, sizeof(struct rejected_message), getpid(), IPC_NOWAIT) > 0){

                printf("transaction rejected user[%d]:{mtype: %ld, amount: %d, user receiver: %d}\n",getpid(), rejected_msg.mtype, rejected_msg.amount, rejected_msg.receiver);
                
                curr_balance = curr_balance + rejected_msg.amount;
            }
            
            /* prendo il semaforo per aggiornare il bilancio*/
            if(semop(SEM_USERS_ID, &sop_p, 1) == -1){
                perror("errore nel semaforo\n");
                return -1;
            }
           /* printf("riservo semaforo per SCRITTURA DATI user: %d\n",getpid());*/
       
            users[USER_ID].budget = curr_balance;
            users[USER_ID].last_block_read++;
            /*printf("rilascio semaforo per SCRITTURA DATI user: %d\n",getpid());*/

            /* rilascio il semaforo*/
            if(semop(SEM_USERS_ID, &sop_r, 1) == -1){
                perror("errore nel semaforo\n");
                return -1;
            }

        }

        if(curr_balance >= 2){

            /*printf("current balance prima: %d , dello user: %d\n", curr_balance, getpid());*/

            do{
                index_ruser= rand() % SO_USERS_NUM;
            }while (index_ruser==USER_ID);
            /* indice random per prendere un nodo a cui inviare la transazione da processare*/
            index_rnode= rand() % SO_NODES_NUM;

            /* calcolo di quanti soldi devo inviare all'altro user*/
            r_number = (rand() % (curr_balance + 1 - 2)) + 2;

            /* calcolo reward */
            calculate_reward = floor(r_number/100*SO_REWARD) ;
            calculate_reward= (calculate_reward>1)? calculate_reward : 1;
            clock_gettime(CLOCK_REALTIME, &timestamp);

            /* Creo la transazione da spedire */
            msg.trans.timestamp = timestamp.tv_nsec;
            msg.trans.sender = getpid();
            msg.trans.receiver = users[index_ruser].pid;
            msg.trans.reward = calculate_reward;
            msg.trans.amount = r_number-calculate_reward;
            msg.mtype = nodes[index_rnode].pid;
            
            /* Variabile per ripetere la transazione in caso di fallimento*/
            retry = SO_RETRY;

            /*#if DEBUG == 1
                printf("\ntransaction user pid: %d:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n", getpid(),msg.trans.timestamp, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);
            #endif*/

            while(retry >= 0){
                if(msgsnd(nodes[index_rnode].id_mq,&msg,sizeof(struct Message),0) < 0) {
                    printf("message send fallita\n");
                    if(retry==0){
                        /* notify to master that retry failed SO_RETRY times */
                        exit(EXIT_FAILURE);
                    }
                    retry--;
                }
                else{
                    
                    printf("\ntransaction user pid: %d:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n", getpid(),msg.trans.timestamp, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);
                    curr_balance = curr_balance - msg.trans.amount - msg.trans.reward;



                    /* prendo il semaforo per aggiornare il bilancio*/
                    if(semop(SEM_USERS_ID, &sop_p, 1) == -1){
                        perror("errore nel semaforo\n");
                        return -1;
                    }
            
                    users[USER_ID].budget = curr_balance;
                
                    /* rilascio il semaforo*/
                    if(semop(SEM_USERS_ID, &sop_r, 1) == -1){
                        perror("errore nel semaforo\n");
                        return -1;
                    }
                    
                    r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
                    timestamp.tv_nsec=r_time;
                    nanosleep(&timestamp, NULL);
                    break;
                };
            }  
        }
        /*else{
            users[USER_ID].status = 0;
        }*/

    }
}
