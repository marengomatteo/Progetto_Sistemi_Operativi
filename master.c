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
/*int sem_nodes_id;
struct sembuf sops;*/
int node_param_id;
int user_param_id;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
int *sh_mem_sources;

char *node_arguments[5];

int *node_pids;

void alarmHandler(int sig)
{
    printf("Allarme ricevuto e trattato\n");
    alarm(1);
}

int main(int argc, char **argv, char **envp)
{
    /* Inizializzo array per i pid dei nodi creati */
    node_pids = malloc(SO_NODES_NUM * sizeof(int));
    
    
   /* if((sem_nodes_id = semget(IPC_PRIVATE, 1, 0600)) == -1)
    {
        printf("Errore nella creazione del semaforo\n");
        exit(EXIT_FAILURE);
    }
    if(semctl(sem_nodes_id, 0, SETVAL, 0) == -1)
    {
        printf("Errore nell'inizializzazione del semaforo\n");
        exit(EXIT_FAILURE);
    }
    sops.sem_num = ID_READY;*/
    /* if (signal(SIGALRM, alarmHandler) == SIG_ERR)
    {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
     }
     alarm(2);*/

    genera_nodi(envp);
    genera_utenti();

    return 0;

}

void genera_nodi(char **envp)
{
    int i;
    char sem_nodes_id_char[10];
    printf("\nGenerazione nodi\n");
    /* SEMAFORO QUI PER I NODI (DOPO LA FORK ASPETTO CHE VENGA GENERATA ALMENO LA CODA DI MESSAGGI/ SETUP INIZIALE DEI NODI) */
   

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

    /*printf("Valore semaforo: %d\n", semctl(sem_nodes_id,1,GETVAL));
    semop(sem_nodes_id, &sops, -1);*/
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
    /*semctl(sem_nodes_id, 0, IPC_RMID, 0);*/
}