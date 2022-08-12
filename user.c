#define _GNU_SOURCE
#include "user.h"
#include <math.h>

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
#define SH_USERS_ID atoi(argv[1])
#define SH_NODES_ID atoi(argv[2])
#define MASTERBOOK_ID atoi(argv[3])
#define USER_ID atoi(argv[4])
#define SEM_ID atoi(argv[5])
#define MASTERBOOK_INFO_ID atoi(argv[6])

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

int index_rnode;
int index_ruser;
int curr_balance;   
int r_number;
int retry;
int calculate_reward;
int r_time;

struct Message {
    long mtype;      
    transaction trans;    
} msg;

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

    /* Current Balance: calcolare TODO*/
    curr_balance = SO_BUDGET_INIT;

    /*Mi aggancio alla memoria condivisa dei nodi*/
    nodes = shmat(SH_NODES_ID, NULL, 0);
    
    /*Mi aggancio alla memoria condivisa degli utenti*/
    users = shmat(SH_USERS_ID, NULL, 0);

    users->last_block_read = 0;

    /*Mi aggancio alla memoria condivisa del masterbook*/
    masterbook = shmat(MASTERBOOK_ID, NULL, 0);

    /* Mi attacco alle memorie condivisa del masterbook */
    shd_masterbook_info = shmat(MASTERBOOK_INFO_ID, NULL, 0);
   
    srand(getpid());

    while (1)
    {    
/*        while(users->last_block_read < shd_masterbook_info->last_block_id){
            printf("leggo blocco %d\n",users->last_block_read );
            for(index_transaction = 0; index_transaction < SO_BLOCK_SIZE; index_transaction++){
                if(masterbook[users->last_block_read].transaction_array[index_transaction].receiver == getpid()){
                    curr_balance += masterbook[users->last_block_read].transaction_array[index_transaction].amount;
                }
            }
            users->last_block_read++;
        }*/

        if(curr_balance >= 2){

            printf("current balance prima: %d , dello user: %d\n", curr_balance, getpid());

            do{
                index_ruser= rand() % SO_USERS_NUM;
            }while (index_ruser==USER_ID);
            
            /* indice random per prendere un nodo a cui inviare la transazione da processare*/
            index_rnode= rand() % SO_NODES_NUM;

            /* calcolo */
            r_number = (rand() % curr_balance-2)+2;

            /* calcolo reward */
            calculate_reward = floor(r_number/100*SO_REWARD);
            
            clock_gettime(CLOCK_REALTIME, &timestamp);

            /* Creo la transazione da spedire */
            msg.trans.timestamp = timestamp.tv_nsec;
            msg.trans.sender = getpid();
            msg.trans.receiver = users[index_ruser].pid;
            msg.trans.reward = (calculate_reward > 1) ? calculate_reward : 1;
            msg.trans.amount = r_number-calculate_reward;
            msg.mtype = nodes[index_rnode].pid;
            
            /* Variabile per ripetere la transazione in caso di fallimento*/
            retry = SO_RETRY;

            #if DEBUG == 1
                printf("\ntransaction user pid: %d:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d}\n", getpid(),msg.trans.timestamp, msg.trans.sender, msg.trans.receiver, msg.trans.amount, msg.trans.reward);

            #endif

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
                    printf("message send eseguita\n");
                    curr_balance = curr_balance - msg.trans.amount - msg.trans.reward;

                    printf("current balance dopo message send: %d , dello user: %d\n", curr_balance, getpid());

                    r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
                    timestamp.tv_nsec=r_time;
                    nanosleep(&timestamp, NULL);
                    break;
                };
            }  
        }

    
    }

    exit(EXIT_SUCCESS);
}
