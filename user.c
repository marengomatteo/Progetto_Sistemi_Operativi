#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include "user.h"
#include "shared.h"
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

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

int curr_balance;   
node_struct* nodes;
user_struct* users;
int index_rnode;
int index_ruser;
int r_number;
int calculate_reward;
struct timespec timestamp;
struct sembuf sops;

struct msgbuf {
    long mtype;      
    transaction* trans;    
} msg;

int main(int argc, char *argv[])
{
    /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
    sops.sem_num = 0;
    sops.sem_op = 0;
    if(semop(SEM_ID, &sops, 1) == -1){
        perror("semaforo rotto");
        exit(EXIT_FAILURE);
    }
    msg.trans= malloc(sizeof(transaction));
    curr_balance=SO_BUDGET_INIT;

    printf("nodes id: %d\n", SH_NODES_ID);
    printf("users id: %d\n", SH_USERS_ID);
    /*Mi aggancio alla memoria condivisa dei nodi*/
    nodes = shmat(SH_NODES_ID, NULL, 0);
    TEST_ERROR;
    
    /*Mi aggancio alla memoria condivisa degli utenti*/
    users = shmat(SH_USERS_ID, NULL, 0);
    TEST_ERROR;
    
    printf("nodes[0].id_mq %d\n", nodes[0].id_mq);

    srand(getpid());
    if(curr_balance>=2){
            index_ruser= rand() % SO_USERS_NUM;
            index_rnode= rand() % SO_NODES_NUM;
            printf("index_rnode: %d\n", index_rnode);
            printf("INDEX NODE: %d", nodes[index_rnode].id_mq);
    
            r_number=(rand() % curr_balance-2)+2;
            calculate_reward=r_number/100*SO_REWARD;
            clock_gettime(CLOCK_REALTIME, &timestamp);
            msg.trans->timestamp= timestamp.tv_nsec;
            msg.trans->sender = getpid();
            msg.trans->receiver = users[index_ruser].pid;
            msg.trans->reward = calculate_reward;
            msg.trans->amount = r_number-calculate_reward;
            msg.mtype=nodes[index_rnode].pid;
            msgsnd(nodes[index_rnode].id_mq,&msg,sizeof(msg),0);
            printf("\nid coda %d\n",nodes[index_rnode].id_mq);
            TEST_ERROR;
         

    }
    printf("\nmain user\n");
}
