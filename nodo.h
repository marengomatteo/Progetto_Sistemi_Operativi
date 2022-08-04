#pragma once
#include "shared.h"


transaction* new_transaction(long timestamp, int sender, int receiver, int amount, int reward){
    transaction *d = malloc(sizeof(transaction));
    d->timestamp = timestamp;
    d->sender = sender;
    d->receiver = receiver;
    d->amount = amount;
    d->reward = reward;
    return d;
}
/* 
typedef struct _node {
  transaction transaction;
  struct _node *next;
} node;

typedef node *list;
 */
void l_add_transaction(transaction *d, list* l){
  node *n = (node*)malloc(sizeof(node));
  n->transaction = *d;
  n->next = *l;
  *l = n;
}


int l_length(list l){
  int length;
  length = 0;

  while(l != NULL){
    length++;
    l = l->next;
  }

  return length;
}



