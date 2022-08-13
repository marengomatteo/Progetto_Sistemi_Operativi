#pragma once
#include "shared.h"

transaction new_transaction(long timestamp, int sender, int receiver, int amount, int reward){
    transaction* d = malloc(sizeof(transaction));
    d->timestamp = timestamp;
    d->sender = sender;
    d->receiver = receiver;
    d->amount = amount;
    d->reward = reward;
    return *d;
}

void l_add_transaction(transaction d, list* l){
  node *n = (node*)malloc(sizeof(node));
  n->transaction = d;
  n->next = *l;
  *l = n;

 #if DEBUG == 0
    printf("\ntransaction in l_add_transaction:{\n\ttimestamp: %ld,\n\tsender: %d,\n\treceiver: %d,\n\tamount: %d,\n\treward: %d\n}\n", d.timestamp, d.sender, d.receiver, d.amount, d.reward);
 #endif
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



