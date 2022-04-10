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
#define NODE_NAME "nodo"
#define USER_NAME "user"
#define ID_READY 0 /* figli pronti: padre puo` procedere */
#define ID_GO 1    /* padre pronto: figli possono procedere */

/* ID IPC Semaforo globale */
int sem_nodes_id;
struct sembuf sops;

int node_param_id;
int user_param_id;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
int *sh_mem_sources;

char *node_arguments[5] = {NODE_NAME};

int *node_pids;

void alarmHandler(int sig)
{
    printf("Allarme ricevuto e trattato\n");
    alarm(1);
}
int main()
{

    /* Inizializzo array per i pid dei nodi creati */
    node_pids = malloc(SO_NODES_NUM * sizeof(int));

    if (signal(SIGALRM, alarmHandler) == SIG_ERR)
    {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }
    alarm(2);
    genera_nodi();
    genera_utenti();
}

void genera_nodi()
{
    int i;

    /* SEMAFORO QUI PER I NODI (DOPO LA FORK ASPETTO CHE VENGA GENERATA ALMENO LA CODA DI MESSAGGI/ SETUP INIZIALE DEI NODI) */
    sem_nodes_id = semget(IPC_PRIVATE, 1, 0600);
    semctl(sem_nodes_id, 0, SETVAL, 1);

    for (i = 0; i < SO_NODES_NUM; i++)
    {
        switch (fork())
        {
        case 0:
            node_pids[i] = getpid();
            /* INSTANZIARE CON EXECVE IL NODO, Passare parametri */
            execve(NODE_NAME, node_arguments, NULL);
            TEST_ERROR;
        case -1:
            TEST_ERROR;
        default:

            break;
        }
    }
}

void genera_utenti()
{
    int i;

    for (i = 0; i < SO_USERS_NUM; i++)
    {
        switch (fork())
        {
        case 0:
            // Passare parametri per creazioni
            printf("pid user: %d", getpid());
            printf("\n");
            exit(EXIT_SUCCESS);
        // execve();
        case -1:
            TEST_ERROR;
        default:

            break;
        }
    }
}