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

#define SO_NODES_NUM atoi(getenv("SO_NODES_NUM"))
#define SO_USERS_NUM atoi(getenv("SO_USERS_NUM"))
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))
#define SO_REWARD atoi(getenv("SO_REWARD"))
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))
#define SO_RETRY atoi(getenv("SO_RETRY"))

int curr_balance;   
node_struct* nodes;
user_struct* users;
int index_rnode;
int index_ruser;
int r_number;
int retry;
int calculate_reward;
int r_time;
struct timespec timestamp;
struct sembuf sops;

struct Message {
    long mtype;      
    transaction trans;    
} msg;

int main(int argc, char *argv[])
{
    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    if(semop(SEM_ID, &sops, 1) == -1){
        perror("semaphore broke");
        exit(EXIT_FAILURE);
    }

    curr_balance=SO_BUDGET_INIT;

    /*Mi aggancio alla memoria condivisa dei nodi*/
    nodes = shmat(SH_NODES_ID, NULL, 0);
    TEST_ERROR;
    
    /*Mi aggancio alla memoria condivisa degli utenti*/
    users = shmat(SH_USERS_ID, NULL, 0);
    TEST_ERROR;

    srand(getpid());

    if(curr_balance>=2){
        index_ruser= rand() % SO_USERS_NUM;
        index_rnode= rand() % SO_NODES_NUM;
        r_number=(rand() % curr_balance-2)+2;
        printf("numero rando: %d\n",r_number);
        calculate_reward=round(r_number/100*SO_REWARD);
        clock_gettime(CLOCK_REALTIME, &timestamp);
        msg.trans.timestamp = timestamp.tv_nsec;
        msg.trans.sender = getpid();
        msg.trans.receiver = nodes[index_rnode].pid;
        msg.trans.reward = (calculate_reward > 1) ? calculate_reward : 1;
        msg.trans.amount = r_number-calculate_reward;
        msg.mtype = nodes[index_rnode].pid;
        retry=SO_RETRY;

        #if DEBUG == 1
            transaction_print(msg.trans);
        #endif

        while(retry >= 0){
            if(msgsnd(nodes[index_rnode].id_mq,&msg,sizeof(struct Message),0) < 0) {
                TEST_ERROR;
                if(retry==0){
                    /* notify to master that retry failed SO_RETRY times */
                    exit(EXIT_FAILURE);
                }
                retry--;
            }
            else break;
        }  
    }

    r_time = (rand()%(SO_MAX_TRANS_GEN_NSEC+1-SO_MIN_TRANS_GEN_NSEC))+SO_MIN_TRANS_GEN_NSEC;
    timestamp.tv_nsec=r_time;
    nanosleep(&timestamp, NULL);

    exit(EXIT_SUCCESS);
}
