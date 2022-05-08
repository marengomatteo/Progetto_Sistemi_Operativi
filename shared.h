#include <time.h>
 #include <sys/shm.h>
typedef struct _transaction {
  long timestamp;
  int sender;
  int receiver;
  int amount;
  int reward;
} transaction;

typedef struct _block{
  int id_block;
  transaction* transaction_array;
} block;
typedef struct _masterbook{
  block* block_transaction;
} masterbook;

int stampaStatoMemoria(int shid) {
  struct shmid_ds buf;
  if (shmctl(shid,IPC_STAT,&buf)==-1) {
    fprintf(stderr, "%s: %d. Errore in shmctl #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
    return -1;
  } else {
  printf("\nSTATISTICHE\n");
  printf("AreaId: %d\n",shid);
  printf("Dimensione: %ld\n",buf.shm_segsz);
  printf("Ultima shmat: %s\n",ctime(&buf.shm_atime));
  printf("Ultima shmdt: %s\n",ctime(&buf.shm_dtime));
  printf("Ultimo processo shmat/shmdt: %d\n",buf.shm_lpid);
  printf("Processi connessi: %ld\n",buf.shm_nattch);
  printf("\n");
  return 0;
  }
}
