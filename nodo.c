#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define SH_PARAM_ID 
#define SM_PARAM_ID atoi(argv[0])

/*Semaforo per segnalare che i nodi sono pronti*/
#define ID_READY 0;

int main(int argc, char *argv[])
{
    struct sembuf sops;
    printf("\ncreato pid nodo: %d\n",getpid());
    printf("semaphore= %d\n\n", SM_PARAM_ID );
    /*coda di messaggi*/
    /* semop con id che punta al semaforo per poter notificare al padre 
    che il nodo ha creato la sua coda di messaggi */
    sops.sem_num = ID_READY;
    sops.sem_op = 1;
    printf("\n%d", SM_PARAM_ID);
    semop(SM_PARAM_ID, &sops, 1);
}