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

#define SO_USERS_NUM 2 /* atoi(getenv("SO_USERS_NUM"))*/
#define SO_NODES_NUM 2 /* atoi(getenv("SO_NODES_NUM"))*/
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
int sem_nodes_id;
struct sembuf sops;
char sem_n_id[3 * sizeof(int) + 1];

int node_param_id;
int user_param_id;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
int shared_nodes_id; /* id memoria condivisa dei nodi*/
int shared_masterbook_id; /* id mlibro mastro*/
int shared_users_id; /* id memoria condivisa degli user*/

char *node_arguments[5] = {NODE_NAME};
char *user_arguments[5] = {USER_NAME};

node_struct *nodes;
masterbook *master_book;
user_struct *user;


void alarmHandler(int sig)
{
    printf("Allarme ricevuto e trattato\n");
    alarm(1);
}

int main(int argc, char **argv, char **envp)
{
    char id_argument_sm_nodes[3 * sizeof(int) + 1]; /*id memoria condivisa nodi*/
    char id_argument_sm_masterbook[3 * sizeof(int) + 1]; /*id memoria condivisa master book*/
    char id_argument_sm_users[3 * sizeof(int) + 1]; /*id memoria condivisa user*/

    /* Create a shared memory area for nodes struct */
    shared_nodes_id = shmget(IPC_PRIVATE, SO_NODES_NUM * sizeof(int), 0600);
    TEST_ERROR;
    /* Attach the shared memory to a pointer */
    nodes = (node_struct *)shmat(shared_nodes_id, NULL, 0);
    TEST_ERROR;

    /* Creazione masterbook */
    shared_masterbook_id = shmget(IPC_PRIVATE, SO_REGISTRY_SIZE * SO_BLOCK_SIZE*sizeof(block), 0600);
    TEST_ERROR;
    master_book =(masterbook*)shmat(shared_masterbook_id, NULL, 0);
    TEST_ERROR;

    /* Creazione memoria condivisa per user*/
    shared_users_id = shmget(IPC_PRIVATE, SO_USERS_NUM*sizeof(int),0600);
    TEST_ERROR;
    user = (user_struct*)shmat(shared_masterbook_id, NULL,0);
    TEST_ERROR;

    /*Converte da int a char gli id delle memorie condivise*/
    sprintf(id_argument_sm_nodes, "%d", shared_nodes_id);
    sprintf(id_argument_sm_masterbook,"%d",shared_masterbook_id);
    sprintf(id_argument_sm_users,"%d",shared_users_id);
    node_arguments[1] = id_argument_sm_nodes;
    user_arguments[1] = id_argument_sm_users;
    node_arguments[4] = id_argument_sm_masterbook;
    user_arguments[4] = id_argument_sm_masterbook;


    genera_nodi(envp);
    genera_utenti(envp);

    /*Rimuovo semaforo*/
    semctl(sem_nodes_id,0, IPC_RMID);
    shmctl(shared_nodes_id,0, IPC_RMID);
    shmctl(shared_masterbook_id,0, IPC_RMID);
    shmctl(shared_users_id,0, IPC_RMID);

    /* if (signal(SIGALRM, alarmHandler) == SIG_ERR)
    {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
     }
     alarm(2);*/
    return 0;
}

void genera_nodi(char **envp)
{
    char node_id[3 * sizeof(int) + 1];
    int i;

    printf("\nGenerazione nodi\n");

    /* SEMAFORO QUI PER I NODI (DOPO LA FORK ASPETTO CHE VENGA GENERATA ALMENO LA CODA DI MESSAGGI/ SETUP INIZIALE DEI NODI) */
    sem_nodes_id = semget(IPC_PRIVATE, 1, 0600);
    TEST_ERROR;
    semctl(sem_nodes_id, 0, SETVAL, 1);
    TEST_ERROR;
    sprintf(sem_n_id, "%d", sem_nodes_id);
    node_arguments[2] = sem_n_id;

    for (i = 0; i < SO_NODES_NUM; i++)
    {
        switch (fork())
        {
        case 0:
           printf("\nCreato nodo %d\n", getpid());
            sprintf(node_id, "%d", i);
            node_arguments[3] = node_id;
            nodes[i].pid = getpid();

            /* INSTANZIARE CON EXECVE IL NODO, Passare parametri */
            if (execve(NODE_NAME, node_arguments, envp) == -1)
                perror("Could not execve");
        case -1:
            TEST_ERROR;
            exit(EXIT_FAILURE);
        default:
            
            sops.sem_num = 0;
            sops.sem_op = -1;
            semop(sem_nodes_id, &sops, 1);
            TEST_ERROR;
            break;
        }
    }

    /*semop(sem_nodes_id, &sops, SO_USERS_NUM);*/
}

void genera_utenti(char** envp)
{
    int i;
    /*semop(sem_nodes_id, &sops, -1);*/
    for (i = 0; i < SO_USERS_NUM; i++)
    {
        switch (fork())
        {
        case 0:
            printf("\nCreato user %d\n", getpid());
            user_arguments[3] = (char)getpid();
            if (execve(USER_NAME, user_arguments, envp) == -1)
                perror("Could not execve");
        case -1:
            TEST_ERROR;
        default:
            break;
        }
    }
}

static void shm_print_stats(int fd, int m_id)
{
    struct shmid_ds my_m_data;
    int ret_val;
    ret_val = shmctl(m_id, IPC_STAT, &my_m_data);

    while (shmctl(m_id, IPC_STAT, &my_m_data))
    {
        TEST_ERROR;
    }
    dprintf(fd, "--- IPC Shared Memory ID: %8d, START ---\n", m_id);
    dprintf(fd, "---------------------- Memory size: %ld\n",
            my_m_data.shm_segsz);
    dprintf(fd, "---------------------- Time of last attach: %ld\n",
            my_m_data.shm_atime);
    dprintf(fd, "---------------------- Time of last detach: %ld\n",
            my_m_data.shm_dtime);
    dprintf(fd, "---------------------- Time of last change: %ld\n",
            my_m_data.shm_ctime);
    dprintf(fd, "---------- Number of attached processes: %ld\n",
            my_m_data.shm_nattch);
    dprintf(fd, "----------------------- PID of creator: %d\n",
            my_m_data.shm_cpid);
    dprintf(fd, "----------------------- PID of last shmat/shmdt: %d\n",
            my_m_data.shm_lpid);
    dprintf(fd, "--- IPC Shared Memory ID: %8d, END -----\n", m_id);
}
