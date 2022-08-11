#include "shared.h"

#define MASTERBOOK_BLOCK_KEY 9826
#define MASTERBOOK_SEM_KEY 1412

masterbook_struct* shd_masterbook_info;
int shared_masterbook_info_id;
int sem_id_block ;


int masterbook_r_init(){

    /* Creazione memoria condivisa per masterbook info*/
    shared_masterbook_info_id = shmget(MASTERBOOK_BLOCK_KEY, sizeof(masterbook_struct), IPC_CREAT | 0666);

    shd_masterbook_info = (masterbook_struct*)shmat(shared_masterbook_info_id, NULL,0);

    /* Inizializzo il semaforo*/
    sem_id_block = semget(MASTERBOOK_SEM_KEY, 1, 0600);

    semctl(sem_id_block, 0, SETVAL, 1);

    return 0;
}

/* richiede un nuovo id  */
int nuovo_id_blocco() {
    
    int block_id;
    struct sembuf sop_p; /* prende la risorsa */
    struct sembuf sop_r; /* rilascia la risorsa */
    
    /* Riservo la risorsa */
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;
    sop_p.sem_flg = 0;

    /* Rilascio la risorsa */
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;
    sop_r.sem_flg = 0;

    if (semop(sem_id_block, &sop_p, 1) == -1) {
        return -1;
    }

    block_id = shd_masterbook_info->last_block_id;
    shd_masterbook_info->last_block_id++;

    /* Rilascio il semaforo */
    if (semop(sem_id_block, &sop_r, 1) == -1) {
        return -1;
    }
    return block_id;
}
