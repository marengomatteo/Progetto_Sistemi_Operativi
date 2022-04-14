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
#define NODE_NAME "./nodo"
#define USER_NAME "./user"
#define ID_READY 0 /* figli pronti: padre puo` procedere */
#define ID_GO 1    /* padre pronto: figli possono procedere */

/* ID IPC Semaforo globale */
int sem_nodes_id;
struct sembuf sops;
int node_param_id;
int user_param_id;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
struct shared_id *sh_mem_sources;
int m_id;

char *node_arguments[5];

int *node_pids;

/*Definire una struct transaction*/
 struct transaction {
    int id;
    int source;
    int dest;
    int amount;
    int status;
};

void alarmHandler(int sig)
{
    printf("Allarme ricevuto e trattato\n");
    alarm(1);
}

int main()
{
  
    /* Inizializzo array per i pid dei nodi creati */
    node_pids = malloc(SO_NODES_NUM * sizeof(int));
	
    /* Create a shared memory area */
    m_id = shmget(IPC_PRIVATE, sizeof(*sh_mem_sources), 0600);
	TEST_ERROR;
	/* Attach the shared memory to a pointer */
	sh_mem_sources = shmat(m_id, NULL, 0);
	TEST_ERROR;

	shm_print_stats(2, m_id);

    genera_nodi();
    genera_utenti();

    /*sem_nodes_id = semget(IPC_PRIVATE, 1, 0600);
    
    if(sem_nodes_id == -1)
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

void genera_nodi()
{
    int i;
    printf("\nGenerazione nodi\n");
    /* SEMAFORO QUI PER I NODI (DOPO LA FORK ASPETTO CHE VENGA GENERATA ALMENO LA CODA DI MESSAGGI/ SETUP INIZIALE DEI NODI) */
    /*sops.sem_num = ID_READY;
    sops.sem_op = 1;*/

    for (i = 0; i < 5; i++)
    {
        
        switch (fork())
        {
            case 0:
                printf("\nCreato nodo %d\n",getpid());
                /*
                  Informo il padre che è nato un nodo
                */
                /*convert sem_nodes_id to char*/
               
                /*sprintf(sem_nodes_id_char,"%d",sem_nodes_id);
                node_arguments[0]=sem_nodes_id_char;
                node_arguments[1]=;*/

                node_pids[i] = getpid();

                /* INSTANZIARE CON EXECVE IL NODO, Passare parametri */
                
                if (execve(NODE_NAME, node_arguments, NULL) == -1)
                    perror("Could not execve");

            case -1:
                TEST_ERROR;
                exit(EXIT_FAILURE);
            default:
               
            break;
        }
    }
   /* semop(sem_nodes_id, &sops, SO_USERS_NUM);*/

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