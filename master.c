#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "master.h"

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
    }

#define SO_USERS_NUM atoi(getenv("SO_USERS_NUM"))
#define SO_NODES_NUM atoi(getenv("SO_NODES_NUM"))
#define SO_REWARD atoi(getenv("SO_REWARD"))
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))
#define SO_RETRY atoi(getenv("SO_RETRY"))
#define SO_TP_SIZE atoi(getenv("SO_TP_SIZE"))
#define SO_BLOCK_SIZE 100
#define SO_MIN_TRANS_PROC_NSEC atoi(getenv("SO_MIN_TRANS_PROC_NSEC"))
#define SO_MAX_TRANS_PROC_NSEC atoi(getenv("SO_MAX_TRANS_PROC_NSEC"))
#define SO_REGISTRY_SIZE 1000
#define SO_SIM_SEC atoi(getenv("SO_SIM_SEC"))
#define SO_FRIENDS_NUM atoi(getenv("SO_FRIENDS_NUM"))
#define SO_HOPS atoi(getenv("SO_HOPS"))

/* Definizione variabili ausiliarie */
#define NODE_NAME "nodo"
#define USER_NAME "user"
#define ID_READY 0 /* figli pronti: padre puo` procedere */
#define ID_GO 1    /* padre pronto: figli possono procedere */

/* ID IPC Semaforo globale */
int sem_nodes_users_id;
struct sembuf sops;
char sem_n_id[3 * sizeof(int) + 1];

int node_param_id;
int user_param_id;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
int shared_nodes_id; /* id memoria condivisa dei nodi*/
int shared_masterbook_id; /* id mlibro mastro*/
int shared_users_id; /* id memoria condivisa degli user*/

char *node_arguments[5] = {NODE_NAME};
char *user_arguments[6] = {USER_NAME};

node_struct *nodes;
masterbook *master_book;
user_struct *user;


void sig_handler(int signum){
   switch(signum){
       case SIGALRM:
         semctl(sem_nodes_users_id,0, IPC_RMID);
         shmctl(shared_nodes_id,0, IPC_RMID);
         shmctl(shared_masterbook_id,0, IPC_RMID);
         shmctl(shared_users_id,0, IPC_RMID);
         kill(getpid(),15);
       break;
       default: 
       break;
   }
   
}

int main(int argc, char **argv, char **envp){

    char id_argument_sm_nodes[3 * sizeof(int) + 1]; /*id memoria condivisa nodi*/
    char id_argument_sm_masterbook[3 * sizeof(int) + 1]; /*id memoria condivisa master book*/
    char id_argument_sm_users[3 * sizeof(int) + 1]; /*id memoria condivisa user*/
    char id_argument_sem_id[3 * sizeof(int) + 1]; /*id semaforo user e nodi*/

    if (signal(SIGALRM, sig_handler)==SIG_ERR) {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }

    alarm(SO_SIM_SEC);

  /* Create a shared memory area for nodes struct */
    shared_nodes_id = shmget(IPC_PRIVATE, SO_NODES_NUM * sizeof(node_struct), 0600);
    TEST_ERROR;
    /* Attach the shared memory to a pointer */
    nodes = (node_struct *)shmat(shared_nodes_id, NULL, 0);
    TEST_ERROR;

    /* Creazione masterbook */
    shared_masterbook_id = shmget(IPC_PRIVATE, SO_REGISTRY_SIZE * SO_BLOCK_SIZE * sizeof(block), 0600);
    TEST_ERROR;
    master_book =(masterbook*)shmat(shared_masterbook_id, NULL, 0);
    TEST_ERROR;

    /* Creazione memoria condivisa per user*/
    shared_users_id = shmget(IPC_PRIVATE, SO_USERS_NUM*sizeof(int),0600);
    TEST_ERROR;
    user = (user_struct*)shmat(shared_masterbook_id, NULL,0);
    TEST_ERROR;

    /* Creazione del semaforo per inizializzare user e nodi*/ 
    sem_nodes_users_id = semget(IPC_PRIVATE, 1, 0600);
    TEST_ERROR;
    sem_init();
    printf("id semaforo: %d\n", sem_nodes_users_id);

    /*Converte da int a char gli id delle memorie condivise*/
    sprintf(id_argument_sm_nodes, "%d", shared_nodes_id);
    sprintf(id_argument_sm_masterbook,"%d",shared_masterbook_id);
    sprintf(id_argument_sm_users,"%d",shared_users_id);
    sprintf(id_argument_sem_id, "%d", sem_nodes_users_id);
    /* Argomenti per nodo*/
    node_arguments[1] = id_argument_sm_nodes;
    node_arguments[2] = id_argument_sem_id;
    node_arguments[4] = id_argument_sm_masterbook;
    /* Argomenti per user*/
    user_arguments[1] = id_argument_sm_users;
    user_arguments[2] = id_argument_sm_nodes;
    user_arguments[3] = id_argument_sm_masterbook;
    user_arguments[5] = id_argument_sem_id;
    printf("memoria condivisa users: %d\n", shared_users_id);
    printf("memoria condivisa nodes: %d\n", shared_nodes_id);

    genera_nodi(envp);
    genera_utenti(envp);

    while(1) {
        printf("manca un secondo in meno\n");
         sleep(1);
    };
   
    return 0;  

}

void genera_nodi(char **envp)
{
    char node_id[3 * sizeof(int) + 1];
    int i;
    int msgq_id;
    printf("\nGenerazione nodi\n");

    for (i = 0; i < SO_NODES_NUM; i++)
    {
        switch (fork())
        {
            case 0:
                printf("\nCreato nodo %d\n", getpid());
                sprintf(node_id, "%d", i);
                node_arguments[3] = node_id;

                /*Inserisco dentro la memoria condivisa dei nodi il pid del nodo e l'id della coda di messaggi*/
                nodes[i].pid = getpid();
                msgq_id = msgget(getpid(), 0600 | IPC_CREAT);
                if(msgq_id == -1){
                    perror("errore sulla coda di messaggi");
                    exit(EXIT_FAILURE);
                }
                nodes[i].id_mq = msgq_id;

                /* INSTANZIARE CON EXECVE IL NODO, Passare parametri */
                if (execve(NODE_NAME, node_arguments, envp) == -1){
                    perror("Could not execve");
                    exit(EXIT_FAILURE);
                }
            case -1:
                TEST_ERROR;
                exit(EXIT_FAILURE);
            default:
                sops.sem_num = 0;
                sops.sem_op = -1;
                sops.sem_flg = 0;
                semop(sem_nodes_users_id, &sops, 1);
                TEST_ERROR;
                break;
        }
    }
}

void genera_utenti(char** envp)
{
    int i;
    char user_id[3*sizeof(int)+1];

    for (i = 0; i < SO_USERS_NUM; i++)
    {
        switch (fork())
        {
            case 0:
                printf("\nCreato user %d\n", getpid());
                sprintf(user_id, "%d", i);
                user_arguments[4] = user_id;

                /*Inserisco dentro la memoria condivisa il pid dello user*/
                user[i].pid=getpid();
 
                if (execve(USER_NAME, user_arguments, envp) == -1){
                    perror("Could not execve");
                    exit(EXIT_FAILURE);
                }
            case -1:
                TEST_ERROR;
                exit(EXIT_FAILURE);
            default:
                sops.sem_num = 0;
                sops.sem_op = -1;
                sops.sem_flg = 0;
                semop(sem_nodes_users_id, &sops, 1);
                TEST_ERROR;
            break;
        }
    }
}

void sem_init(){
    semctl(sem_nodes_users_id, 0, SETVAL, SO_NODES_NUM+SO_USERS_NUM);
}
