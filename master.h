/*L'uso di #pragma once può ridurre i tempi di compilazione, 
perché il compilatore non apre e legge nuovamente il file #include dopo il primo file 
nell'unità di conversione*/
#pragma once 

#include "shared.h"

typedef struct node_struct {
    int pid;
    int id_mq;
} node_struct;

typedef struct user_struct{
    int pid;
} user_struct;

void genera_nodi(char ** envp);
void genera_utenti();
static void set_shared_memory();
static void shm_print_stats(int fd, int m_id);
