/*L'uso di #pragma once può ridurre i tempi di compilazione, 
perché il compilatore non apre e legge nuovamente il file #include dopo il primo file 
nell'unità di conversione*/
#pragma once 

#define SO_USERS_NUM atoi(getenv("SO_USERS_NUM"))
#define SO_NODES_NUM atoi(getenv("SO_NODES_NUM"))

void genera_nodi();
void genera_utenti();
static void shm_print_stats(int fd, int m_id);

struct shared_id{
    pid_t users_id[10];  
	pid_t nodes_id[10];
};
