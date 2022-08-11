#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define DEBUG 1

#define SO_BLOCK_SIZE 3
#define SO_REGISTRY_SIZE 1000

typedef struct _transaction {
  long timestamp;
  int sender;
  int receiver;
  int amount;
  int reward;
} transaction;

typedef struct _node {
  transaction transaction;
  struct _node *next;
} node;

typedef node *list;

typedef struct _block{
  int id_block;
  struct _transaction transaction_array[SO_BLOCK_SIZE];
} block;

typedef struct _node_struct {
    int pid;
    int id_mq;
} node_struct;

typedef struct _user_struct{
    int pid;
    int budget;
    int status; /* 0 dead, 1 alive */
    int last_block_read;
} user_struct;

typedef struct _masterbook_struct {
  int last_block_id;
  int block_count;
} masterbook_struct;

/*
int masterbook_r_init();
int nuovo_id_blocco();*/

void transaction_print (transaction d){
  printf("transaction:{\n\ttimestamp: %ld,\n\tsender: %d,\n\treceiver: %d,\n\tamount: %d,\n\treward: %d\n},\n", d.timestamp, d.sender, d.receiver, d.amount, d.reward);
}

void l_print(list l){
  	printf("\nlista transazioni:\n{\n");
	for ( ; l!=NULL ; l=l->next){
    transaction_print(l->transaction);
  }
	  printf("\b\b},\n");
}

void a_print(block l){
  int i;  
	printf("\nmasterbook:\n{\n");
	for ( i = 0; i < SO_BLOCK_SIZE ; i++){
    transaction_print(l.transaction_array[i]);
  }
	  printf("\b\b},\n");
}


/*Da rivedere mettendo tutto in shared.c*/
#define MASTERBOOK_BLOCK_KEY 9826
#define MASTERBOOK_SEM_KEY 1412

masterbook_struct* shd_masterbook_info;
int shared_masterbook_info_id;
int sem_id_block ;


int masterbook_r_init(){

    printf("init masterbook");
    /* Creazione memoria condivisa per masterbook info*/
    shared_masterbook_info_id = shmget(MASTERBOOK_BLOCK_KEY, sizeof(masterbook_struct), IPC_CREAT | 0666);

    shd_masterbook_info = (masterbook_struct*)shmat(shared_masterbook_info_id, NULL,0);

    /* Inizializzo il semaforo*/
    sem_id_block = semget(MASTERBOOK_SEM_KEY, 1, 0600);

    semctl(sem_id_block, 0, SETVAL, 1);

    return 0;
}

/* richiede un nuovo id  */
int nuovo_id_blocco() {
    
    int block_id;
    struct sembuf sop_p; /* prende la risorsa */
    struct sembuf sop_r; /* rilascia la risorsa */
    
    /* Riservo la risorsa */
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;
    sop_p.sem_flg = 0;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;
    sop_r.sem_flg = 0;

    if (semop(sem_id_block, &sop_p, 1) == -1) {
        return -1;
    }

    block_id = shd_masterbook_info->last_block_id;
    shd_masterbook_info->last_block_id++;

    /* Rilascio il semaforo */
    if (semop(sem_id_block, &sop_r, 1) == -1) {
        return -1;
    }
    return block_id;
}