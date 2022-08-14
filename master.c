#define _GNU_SOURCE

#include "master.h"

#define SO_USERS_NUM atoi(getenv("SO_USERS_NUM"))
#define SO_NODES_NUM atoi(getenv("SO_NODES_NUM"))
#define SO_REWARD atoi(getenv("SO_REWARD"))
#define SO_MIN_TRANS_GEN_NSEC atoi(getenv("SO_MIN_TRANS_GEN_NSEC"))
#define SO_MAX_TRANS_GEN_NSEC atoi(getenv("SO_MAX_TRANS_GEN_NSEC"))
#define SO_RETRY atoi(getenv("SO_RETRY"))
#define SO_TP_SIZE atoi(getenv("SO_TP_SIZE"))
#define SO_MIN_TRANS_PROC_NSEC atoi(getenv("SO_MIN_TRANS_PROC_NSEC"))
#define SO_MAX_TRANS_PROC_NSEC atoi(getenv("SO_MAX_TRANS_PROC_NSEC"))
#define SO_SIM_SEC atoi(getenv("SO_SIM_SEC"))
#define SO_FRIENDS_NUM atoi(getenv("SO_FRIENDS_NUM"))
#define SO_HOPS atoi(getenv("SO_HOPS"))
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))

/* Definizione variabili ausiliarie */
#define NODE_NAME "nodo"
#define USER_NAME "user"

/* ID IPC Semaforo globale */
int sem_users_id;
int sem_nodes_users_id;
int sem_masterbook_id;

struct sembuf sops;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
int shared_nodes_id; /* id memoria condivisa dei nodi*/
int shared_masterbook_id; /* id mlibro mastro*/
int shared_users_id; /* id memoria condivisa degli user*/

char *node_arguments[6] = {NODE_NAME};
char *user_arguments[8] = {USER_NAME};

struct timespec timestamp;
node_struct *nodes;
block *master_book;
user_struct *user;
masterbook_struct* shd_masterbook_info;
int shared_masterbook_info_id;

void sig_handler(int signum){
   switch(signum){
        case SIGALRM:
            remove_users();
            remove_nodes();
            remove_IPC();
            kill(getpid(),15);
        break;
        default:
        break;
   }
}

int main(int argc, char **argv, char **envp){

    char id_argument_sm_nodes[3 * sizeof(int) + 1]; /*id memoria condivisa nodi*/
    char id_argument_sm_users[3 * sizeof(int) + 1]; /*id memoria condivisa user*/
    char id_argument_sm_masterbook[3 * sizeof(int) + 1]; /*id memoria condivisa master book*/
    char id_argument_sm_masterinfo[3 * sizeof(int) + 1]; /*id memoria condivisa masterbook*/

    char id_argument_sem_id[3 * sizeof(int) + 1]; /*id semaforo user e nodi*/
    char id_argument_sem_users_id[3*sizeof(int)+1]; /*id semaforo per user*/
    char id_argument_sem_masterbook_id[3 * sizeof(int) + 1]; /*id semaforo per scrivere e leggere su masterbook*/

    struct timespec* time;
    if (signal(SIGALRM, sig_handler)==SIG_ERR) {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }

    if(SO_TP_SIZE <= SO_BLOCK_SIZE) {
        printf("Parametri errati\n");
        exit(EXIT_FAILURE);
    }
   
    alarm(10);

    /* Create a shared memory area for nodes struct */
    shared_nodes_id = shmget(IPC_PRIVATE, SO_NODES_NUM * sizeof(node_struct), 0600);
    if(shared_nodes_id == -1){
        printf("Errore durante la creazione della shared memory: %s\n",strerror(errno));
        return -1;
    }
    /* Attach the shared memory to a pointer */
    nodes = (node_struct *)shmat(shared_nodes_id, NULL, 0);

    /* Creazione masterbook */
    shared_masterbook_id = shmget(IPC_PRIVATE, SO_REGISTRY_SIZE * SO_BLOCK_SIZE * sizeof(block), 0600);
    if(shared_masterbook_id == -1){
        printf("Errore durante la creazione della shared memory: %s\n",strerror(errno));
        return -1;
    }
    master_book =(block*)shmat(shared_masterbook_id, NULL, 0);

    /* Creazione memoria condivisa per user*/
    shared_users_id = shmget(IPC_PRIVATE, SO_USERS_NUM*sizeof(user_struct),0600);
    if(shared_users_id == -1){
        printf("Errore durante la creazione della shared memory: %s\n",strerror(errno));
        return -1;
    }
    user = (user_struct*)shmat(shared_users_id, NULL,0);

    /* Creazione memoria condivisa per info masterbook */
    shared_masterbook_info_id = shmget(IPC_PRIVATE, sizeof(masterbook_struct), 0600);
    if(shared_masterbook_info_id == -1){
        printf("Errore durante la creazione della shared memory: %s\n",strerror(errno));
        return -1;
    }

    shd_masterbook_info = (masterbook_struct*)shmat(shared_masterbook_info_id, NULL,0);
    shd_masterbook_info->last_block_id = 0;

    /* Inizializzo il semaforo*/
    shd_masterbook_info->sem_masterbook = semget(getpid(), 1,IPC_CREAT | 0600);
    if(shd_masterbook_info->sem_masterbook == -1){
        printf("Errore durante la creazione del semaforo delle informazioni sul masterbook: %s\n",strerror(errno));
        return -1;
    }    

    /* Creazione del semaforo per inizializzare user e nodi */ 
    sem_nodes_users_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (sem_nodes_users_id == -1) {
    	printf("Errore durante la creazione del semaforo users e nodes: %s\n",strerror(errno));
        return -1;
	}

    /* Creazione del semaforo per poter scrivere sul masterbook */ 
    sem_masterbook_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (sem_masterbook_id == -1) {
    	printf("Errore durante la creazione del semaforo users e nodes: %s\n",strerror(errno));
        return -1;
	}
    
    /* Creazione del semaforo per poter accedere alle informazioni dello user */
    sem_users_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if(sem_users_id == -1){
        printf("Errore durante la creazione del semaforo per accedere alle informazioni dello user: %s\n", strerror(errno));
        return -1;
    }

    /* Inizializzazione semafori */
    sem_init();

    /* Converte da int a char gli id delle memorie condivise */
    sprintf(id_argument_sm_nodes, "%d", shared_nodes_id);
    sprintf(id_argument_sm_masterbook,"%d",shared_masterbook_id);
    sprintf(id_argument_sm_users,"%d",shared_users_id);
    sprintf(id_argument_sm_masterinfo,"%d",shared_masterbook_info_id);

    sprintf(id_argument_sem_id, "%d", sem_nodes_users_id);
    sprintf(id_argument_sem_masterbook_id,"%d",sem_masterbook_id);
    sprintf(id_argument_sem_users_id,"%d",sem_users_id);

    /* Argomenti per nodo*/
    node_arguments[0] = id_argument_sm_nodes;
    node_arguments[1] = id_argument_sem_id;
    node_arguments[3] = id_argument_sm_masterbook;
    node_arguments[4] = id_argument_sm_masterinfo;
    node_arguments[5] = id_argument_sem_masterbook_id;

    /* Argomenti per user*/
    user_arguments[0] = id_argument_sm_users;
    user_arguments[1] = id_argument_sm_nodes;
    user_arguments[2] = id_argument_sm_masterbook;
    user_arguments[4] = id_argument_sem_id;
    user_arguments[5] = id_argument_sm_masterinfo;
    user_arguments[6] = id_argument_sem_masterbook_id;
    user_arguments[7] = id_argument_sem_users_id;


    #if DEBUG == 1
        printf("memoria condivisa users: %d\n", shared_users_id);
        printf("memoria condivisa nodes: %d\n", shared_nodes_id);
    #endif

    genera_nodi(envp);
    genera_utenti(envp);

    sops.sem_num = 0;
    sops.sem_op = 0;
    printf("Attendo utenti e nodi\n");
    semop(sem_nodes_users_id, &sops, 1);
    printf("Finito utenti e nodi\n");

    while(1){
        stampa_info();
        sleep(1);
    }
      

    timestamp.tv_sec=1;
    while(1) {
        printf("manca un secondo in meno\n");
        nanosleep(&timestamp, NULL);
    };

    return 0;  
}

void genera_nodi(char **envp)
{
    char node_id[3 * sizeof(int) + 1];
    int i;
    int msgq_id;
   
    for (i = 0; i < SO_NODES_NUM; i++)
    {
        switch (fork())
        {
            case 0:
                /*printf("\nCreato nodo %d\n", getpid());
                */
               sprintf(node_id, "%d", i);
                node_arguments[2] = node_id;

                /*Inserisco dentro la memoria condivisa dei nodi il pid del nodo e l'id della coda di messaggi*/
                nodes[i].pid = getpid();
                nodes[i].budget=0;
                nodes[i].status=1;

                msgq_id = msgget(getpid(), 0600 | IPC_CREAT);
                if(msgq_id == -1){
                    perror("errore sulla coda di messaggi");
                    exit(EXIT_FAILURE);
                }
                nodes[i].id_mq = msgq_id;

                sops.sem_num = 0;
                sops.sem_op = -1;
                sops.sem_flg = 0;
                semop(sem_nodes_users_id, &sops, 1);
                /* INSTANZIARE CON EXECVE IL NODO, Passare parametri */
                if (execve(NODE_NAME, node_arguments, envp) == -1){
                    perror("Could not execve");
                    exit(EXIT_FAILURE);
                }
            case -1:
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }
}

void genera_utenti(char** envp)
{
    int i;
    char user_id[3*sizeof(int)+1];

    for (i = 0; i < SO_USERS_NUM; i++)
    {
        switch (fork())
        {
            case 0:
                /*printf("\nCreato user %d\n", getpid());*/
                sprintf(user_id, "%d", i);
                user_arguments[3] = user_id;

                /*Inserisco dentro la memoria condivisa il pid dello user*/
                user[i].pid=getpid();
                user[i].last_block_read = 0;
                user[i].budget = SO_BUDGET_INIT;
                user[i].status = 1;

                sops.sem_num = 0;
                sops.sem_op = -1;
                sops.sem_flg = 0;
                if(semop(sem_nodes_users_id, &sops, 1)<0){
                    printf("Errore semaforo\n");
                }
               
                if (execve(USER_NAME, user_arguments, envp) == -1){
                    perror("Could not execve");
                    exit(EXIT_FAILURE);
                }
            case -1:
                exit(EXIT_FAILURE);
            default:
            break;
        }
    }
}

void sem_init(){
    semctl(sem_nodes_users_id, 0, SETVAL, SO_NODES_NUM+SO_USERS_NUM);
    semctl(sem_masterbook_id, 0, SETVAL, 1);
    semctl(shd_masterbook_info->sem_masterbook, 0, SETVAL, 1);
    semctl(sem_users_id, 0, SETVAL, 1);
}

void remove_IPC(){
    int i;
    for (i=0;i<SO_NODES_NUM ;i++)
    {
        msgctl(nodes[i].id_mq, IPC_RMID, NULL);
    }
    semctl(sem_nodes_users_id,0, IPC_RMID);
    semctl(sem_users_id, 0, IPC_RMID);
    semctl(sem_masterbook_id, 0, IPC_RMID);
    semctl(shd_masterbook_info->sem_masterbook, 0, IPC_RMID);
    shmctl(shared_nodes_id,0, IPC_RMID);
    shmctl(shared_masterbook_id,0, IPC_RMID);
    shmctl(shared_users_id,0, IPC_RMID);
    shmctl(shared_masterbook_info_id, 0, IPC_RMID);
}

void remove_users(){
    int i;
    for( i = 0; i < SO_USERS_NUM; i++){
        if(user[i].status == 1)
            kill(user[i].pid, 9);
    }
}

void remove_nodes(){
    int i;
    for( i = 0; i < SO_NODES_NUM; i++){
        kill(nodes[i].pid, 9);
    }
}

void stampa_info(){
    int i,user_alive=0, nodes_alive=0;
    for(i=0; i<SO_USERS_NUM;i++){
        if(user[i].status==1)user_alive++;
    }
    if(user_alive==0){
        printf("User terminati--KILL");
        /* kill(); */
    }
    for(i=0; i<SO_NODES_NUM;i++){
        if(nodes[i].status==1)nodes_alive++;
    }
    printf("\nNODI ATTIVI: %d\tUSER ATTIVI: %d\n", nodes_alive,user_alive);
    stampa_nodi();
    stampa_utenti();
}
void stampa_utenti(){
    int i;
    struct sembuf sop_p;
    struct sembuf sop_r;

    /* prendo la risorsa*/
    sop_p.sem_num = 0;
    sop_p.sem_op = -1;

    /* rilascio la risorsa*/
    sop_r.sem_num = 0;
    sop_r.sem_op = 1;

    for(i = 0; i < SO_USERS_NUM; i++){
        if(semop(sem_users_id, &sop_p, 1) == -1){
            perror("errore nel semaforo\n");
        }
        /*printf("riservo semaforo per LETTURA DATI user: DA MASTER\n");*/

        printf("user:{ pid: %d, budget: %d, status: %d}\n", user[i].pid, user[i].budget, user[i].status);
        /*printf("rilascio semaforo per LETTURA DATI user: DA MASTER\n");*/

        if(semop(sem_users_id, &sop_r, 1) == -1){
            perror("errore nel semaforo\n");
        }
    }
  
}

void stampa_nodi(){
    int i;
    for( i = 0; i < SO_NODES_NUM; i++){
        printf("node:{ pid: %d, budget: %d}\n", nodes[i].pid, nodes[i].budget);
    }
       
}