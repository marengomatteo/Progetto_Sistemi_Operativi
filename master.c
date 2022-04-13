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

/*Definire una struct transaction*/

void alarmHandler(int sig)
{
    printf("Allarme ricevuto e trattato\n");
    alarm(1);
}

int main()
{
    printf("%s",getenv("HOME"));

    /* Inizializzo array per i pid dei nodi creati */
    /* node_pids =(int*) malloc(SO_NODES_NUM * sizeof(int));
    */
   
    // genera_nodi();

    /* if (signal(SIGALRM, alarmHandler) == SIG_ERR)
    {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
     }
     alarm(2);*/
    /*genera_utenti();*/
}

void genera_nodi()
{
    int i;
    printf("\nGenerazione nodi\n");
    /* SEMAFORO QUI PER I NODI (DOPO LA FORK ASPETTO CHE VENGA GENERATA ALMENO LA CODA DI MESSAGGI/ SETUP INIZIALE DEI NODI) */
    sem_nodes_id = semget(IPC_PRIVATE, 5, 0600);
    if(sem_nodes_id == -1)
    {
        printf("Errore nella creazione del semaforo\n");
        exit(EXIT_FAILURE);
    }
    if(semctl(sem_nodes_id, 0, SETVAL, 0) == -1)
    {
        printf("Errore nell'inizializzazione del semaforo\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 5; i++)
    {
        
        switch (fork())
        {
            case 0:
                printf("\nCreato nodo %d\n",getpid());
                /*
                  Informo il padre che è nato un nodo
                */
                sops.sem_num = ID_READY;
                sops.sem_op = 0;

                if(semop(sem_nodes_id, &sops, 1) == -1)
                {
                    printf("Errore nell'operazione di decremento del semaforo\n");
                    exit(EXIT_FAILURE);
                }
                printf("superata if\n");
                node_arguments[1] = 0;
                char sem_nodes_id_char[10];
                sprintf(sem_nodes_id_char,"%c",sem_nodes_id);
                node_arguments[2] = sem_nodes_id_char;

                node_pids[i] = getpid();
                printf("superato node_arguments e pid\n");

                /* INSTANZIARE CON EXECVE IL NODO, Passare parametri */
                execve("./nodo", node_arguments, NULL);
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