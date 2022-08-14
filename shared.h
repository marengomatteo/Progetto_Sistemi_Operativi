#ifndef __SHARED_H__
#define __SHARED_H__

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
#define ID_QUEUE_MESSAGE_REJECTED 20

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

typedef struct _list{
  node* head;
  node* tail;
}list;

typedef struct _block{
  int id_block;
  struct _transaction transaction_array[SO_BLOCK_SIZE];
} block;

typedef struct _node_struct {
    int pid;
    int id_mq;
    int budget;
    int status;
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
  int sem_masterbook;
} masterbook_struct;


void transaction_print (transaction d){
  printf("transaction:{timestamp: %ld,sender: %d,receiver: %d,amount: %d,reward: %d},\n", d.timestamp, d.sender, d.receiver, d.amount, d.reward);
}

void l_print(list l){
    node* tmp=l.head;
  	printf("\n-----lista transazioni:-----\n");
	for ( ; tmp!=NULL ; tmp=tmp->next){
    transaction_print(tmp->transaction);
  }
	  printf("\n-------\n");
}

void a_print(block l){
  int i;  
	printf("blocco:\n{\n");
	for ( i = 0; i < SO_BLOCK_SIZE ; i++){
    transaction_print(l.transaction_array[i]);
  }
	  printf("\b\b},\n");
}

void masterbook_print(block* blocks){
  int i;
  for(i = 0; i < SO_BLOCK_SIZE; i++){
    a_print(blocks[i]);
  }
}

#endif