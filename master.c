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

#define SO_USERS_NUM 2 //atoi(getenv("SO_USERS_NUM"))
#define SO_NODES_NUM 2 //atoi(getenv("SO_NODES_NUM"))
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))
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
#define USER_NAME "./user"
#define ID_READY 0 /* figli pronti: padre puo` procedere */
#define ID_GO 1    /* padre pronto: figli possono procedere */

/* ID IPC Semaforo globale */
int sem_nodes_id;
struct sembuf sops;
char sem_n_id[3 * sizeof(int) + 1];

int node_param_id;
int user_param_id;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
int shared_nodes_id;

char *node_arguments[5] = {NODE_NAME};

node_struct *nodes;

void alarmHandler(int sig)
{
    printf("Allarme ricevuto e trattato\n");
    alarm(1);
}

int main(int argc, char **argv, char **envp)
{

    char id_nodes[3 * sizeof(int) + 1];

    /* Create a shared memory area for nodes struct */
    shared_nodes_id = shmget(IPC_PRIVATE, SO_NODES_NUM * sizeof(int), 0600);
	TEST_ERROR;
	/* Attach the shared memory to a pointer */
	nodes = (node_struct *) shmat(shared_nodes_id, NULL, 0);
	TEST_ERROR;

    sprintf(id_nodes, "%d", shared_nodes_id);

    node_arguments[1] = id_nodes;

    genera_nodi(envp);
    genera_utenti();

    /*sem_nodes_id = semget(IPC_PRIVATE, 1, 0600);
    
   /* if((sem_nodes_id = semget(IPC_PRIVATE, 1, 0600)) == -1)
    {
        printf("Errore nella creazione del semaforo\n");
        exit(EXIT_FAILURE);
    }
    if(semctl(sem_nodes_id, 0, SETVAL, 0) == -1)
    {
        printf("Errore nell'inizializzazione del semaforo\n");
        exit(EXIT_FAILURE);
    }*/

    /* if (signal(SIGALRM, alarmHandler) == SIG_ERR)
    {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
     }
     alarm(2);*/

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
                printf("\nCreato nodo %d\n",getpid());
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
               
            break;
        }
    }
    /*semop(sem_nodes_id, &sops, SO_USERS_NUM);*/



}

void genera_utenti()
{
    int i;
   /* semop(sem_nodes_id, &sops, -1);*/
    for (i = 0; i < SO_USERS_NUM; i++)
    {
        switch (fork())
        {
            case 0:
            /*operazione su semaforo -1*/
                /* Passare parametri per creazioni*/
                printf("pid user: %d", getpid());
                printf("\n");
                exit(EXIT_SUCCESS);
            /* execve();*/
            case -1:
                TEST_ERROR;
            default:

                break;
        }
    }
}



static void shm_print_stats(int fd, int m_id) {
	struct shmid_ds my_m_data;
	int ret_val;
	ret_val = shmctl(m_id, IPC_STAT, &my_m_data);

	while (shmctl(m_id, IPC_STAT, &my_m_data)) {
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