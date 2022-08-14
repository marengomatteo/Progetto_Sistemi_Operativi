#pragma once
#include "shared.h"

int nuovo_id_blocco(int sem_id);
int l_length(list l);
void l_add_transaction(transaction d, list* l);
transaction reward_transaction(long timestamp, int sender, int receiver, int amount, int reward);



