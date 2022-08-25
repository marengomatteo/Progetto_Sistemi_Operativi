#pragma once
#include "shared.h"

int new_id_block(int sem_id);
int l_length(list l);
void l_add_transaction(transaction d, list* l);
transaction create_reward_transaction(long timestamp_nsec, long timestamp_sec, int sender, int receiver, int amount, int reward);
void build_block(int node_id, int sem_nodes);
