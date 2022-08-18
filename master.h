/*L'uso di #pragma once può ridurre i tempi di compilazione, 
perché il compilatore non apre e legge nuovamente il file #include dopo il primo file 
nell'unità di conversione*/
#pragma once 

#include "shared.h"

int ipc_init();
void sem_init();
void remove_IPC();
void genera_nodi(char ** envp);
void genera_utenti(char ** envp);
static void set_shared_memory();
static void shm_print_stats(int fd, int m_id);
void stampa_info();
void stampa_utenti();
void stampa_nodi();
void remove_nodes();
void remove_users();
void stampa_review_finale(int exit_reason);
void get_friends(int nodes_id);
void print_min_max_budget_user();
