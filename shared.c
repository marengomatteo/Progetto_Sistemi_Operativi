#include "shared.h"

#define MASTERBOOK_BLOCK_KEY 9826
#define MASTERBOOK_SEM_ID 10

masterbook_struct* shd_masterbook_info;
int shared_masterbook_info_id;
int sem_id_block;


int masterbook_r_init(int create){

    printf("init masterbook\n");
    /* Creazione memoria condivisa per masterbook info*/
    shared_masterbook_info_id = shmget(MASTERBOOK_BLOCK_KEY, sizeof(masterbook_struct), create == 1 ? IPC_CREAT | 0666 : 0666);
    if(shared_masterbook_info_id == -1){
        printf("Errore durante la creazione della shared memory: %s\n",strerror(errno));
        return -1;
    }

    shd_masterbook_info = (masterbook_struct*)shmat(shared_masterbook_info_id, NULL,0);
    shd_masterbook_info->last_block_id = 0;
    /* Inizializzo il semaforo*/
    sem_id_block = semget(MASTERBOOK_SEM_ID, 1, create == 1 ? IPC_CREAT | 0666 : 0666);
     if(sem_id_block == -1){
        printf("Errore durante la creazione del semaforo: %s\n",strerror(errno));
        return -1;
    }
    if(create==1)semctl(sem_id_block, 0, SETVAL, 1);

    return 0;
}

/* richiede un nuovo id  */
/* int nuovo_id_blocco() {
    
    int block_id;
    struct sembuf sop_p; /* prende la risorsa 
    struct sembuf sop_r; /* rilascia la risorsa 
    
    /* Riservo la risorsa  
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;
    sop_p.sem_flg = 0;

    /* Rilascio la risorsa 
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;
    sop_r.sem_flg = 0;

    printf("prendo id\n");
    if (semop(sem_id_block, &sop_p, 1) == -1) {
        return -1;
    }

    printf("shd_masterbook_info->last_block_id: %d\n",shd_masterbook_info->last_block_id);

    block_id = shd_masterbook_info->last_block_id;
    shd_masterbook_info->last_block_id++;

    Rilascio il semaforo 
    if (semop(sem_id_block, &sop_r, 1) == -1) {
        return -1;
    }
    printf("lascio  id");

    return block_id;
}*/


