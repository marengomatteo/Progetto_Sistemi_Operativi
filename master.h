/*L'uso di #pragma once può ridurre i tempi di compilazione, 
perché il compilatore non apre e legge nuovamente il file #include dopo il primo file 
nell'unità di conversione*/
#pragma once 

/*Definizione di una struct transaction*/
typedef struct transaction {
    int timestamp;
    int sender;
    int receiver;
    int amount;
    int reward;
} transaction;

typedef struct node_struct {
    int pid;
    int id_mq;
} node_struct;

void genera_nodi(char ** envp);
void genera_utenti();
static void shm_print_stats(int fd, int m_id);