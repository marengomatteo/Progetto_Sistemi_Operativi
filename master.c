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
#define SO_HOPS atoi(getenv("SO_HOPS"))
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))
#define SO_NUM_FRIENDS atoi(getenv("SO_NUM_FRIENDS"))

#define MAX_PROC_PRINTABLE 10

/* Definizione variabili ausiliarie */
#define NODE_NAME "nodo"
#define USER_NAME "user"

/* ID IPC Semaforo globale */
int sem_users_id;
int sem_nodes_users_id;
int sem_masterbook_id;

struct sembuf sops;

struct sembuf sop_p;
struct sembuf sop_r;
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

int exitReason=2, user_dead=0, nodes_dead=0;
void sig_handler(int signum){
   switch(signum){
        case SIGALRM:
            exitReason = 0;
            break;
        case SIGINT:
            break;
        case SIGUSR1:
            exitReason = 3;
            break;
        default:
        break;
   }
   stampa_review_finale(exitReason);
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
    if (signal(SIGINT, sig_handler)==SIG_ERR) {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGUSR1, sig_handler)==SIG_ERR) {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }

    if(SO_TP_SIZE <= SO_BLOCK_SIZE) {
        printf("Parametri errati\n");
        exit(EXIT_FAILURE);
    }
   
    alarm(20);

    /* init memorie condivise */
    if(ipc_init() < 0){
        exit(EXIT_FAILURE);
    }

    /* Inizializzazione semafori */
    sem_init();
    sop_p.sem_num=0;
    sop_p.sem_op=-1;

    sop_r.sem_num=0;
    sop_r.sem_op=1;
    

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
    
    semop(sem_nodes_users_id, &sops, 1);

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

int ipc_init(){

    /* Create a shared memory area for nodes struct */
    shared_nodes_id = shmget(IPC_PRIVATE, SO_NODES_NUM * sizeof(node_struct), 0600);
    if(shared_nodes_id == -1){
        perror("Errore durante la creazione della shared memory dei nodi\n");
        return -1;
    }
    /* Attach the shared memory to a pointer */
    nodes = (node_struct *)shmat(shared_nodes_id, NULL, 0);

    /* Creazione masterbook */
    shared_masterbook_id = shmget(IPC_PRIVATE, SO_REGISTRY_SIZE * SO_BLOCK_SIZE * sizeof(block), 0600);
    if(shared_masterbook_id == -1){
        perror("Errore durante la creazione della shared memory del masterbook\n");
        return -1;
    }
    master_book =(block*)shmat(shared_masterbook_id, NULL, 0);

    /* Creazione memoria condivisa per user*/
    shared_users_id = shmget(IPC_PRIVATE, SO_USERS_NUM*sizeof(user_struct),0600);
    if(shared_users_id == -1){
        perror("Errore durante la creazione della shared memory degli user\n");
        return -1;
    }
    user = (user_struct*)shmat(shared_users_id, NULL,0);

    /* Creazione memoria condivisa per info masterbook */
    shared_masterbook_info_id = shmget(IPC_PRIVATE, sizeof(masterbook_struct), 0600);
    if(shared_masterbook_info_id == -1){
        perror("Errore durante la creazione della shared memory per info masterbook\n");
        return -1;
    }

    shd_masterbook_info = (masterbook_struct*)shmat(shared_masterbook_info_id, NULL,0);
    shd_masterbook_info->last_block_id = 0;
    shd_masterbook_info->num_block = 0;

    /* Inizializzo il semaforo*/
    shd_masterbook_info->sem_masterbook = semget(getpid(), 1,IPC_CREAT | 0600);
    #if DEBUG == 1
        printf("semaforo creato: %d\n", shd_masterbook_info->sem_masterbook);
    #endif

    if(shd_masterbook_info->sem_masterbook == -1){
        perror("Errore durante la creazione del semaforo delle informazioni sul masterbook\n");
        return -1;
    }    

    /* Creazione del semaforo per inizializzare user e nodi */ 
    sem_nodes_users_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (sem_nodes_users_id == -1) {
    	perror("Errore durante la creazione del semaforo users e nodes\n");
        return -1;
	}

    /* Creazione del semaforo per poter scrivere sul masterbook */ 
    sem_masterbook_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (sem_masterbook_id == -1) {
    	perror("Errore durante la creazione del semaforo per scrivere sul masterbook\n");
        return -1;
	}
    
    /* Creazione del semaforo per poter accedere alle informazioni dello user */
    sem_users_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if(sem_users_id == -1){
        perror("Errore durante la creazione del semaforo per accedere alle informazioni dello user\n");
        return -1;
    }
}


void genera_nodi(char **envp)
{
    char node_id[3 * sizeof(int) + 1];
    int i,j;
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
                nodes[i].tp_size=0;
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

                /* semop in attesa che tutti i nodi e gli utenti vengano creati*/
                printf("Attendo tutti i nodi e utenti\n");
                sops.sem_op = 0;
                sops.sem_num = 0;
                semop(sem_nodes_users_id, &sops, 1);
                printf("Creo amici\n");
                nodes[i].friends = get_friends(getpid());
                printf("sono in for\n");
                for( j=0; j<SO_NUM_FRIENDS;j++){
                    printf("Amico %d: %d\n", j, nodes[i].friends[j]);
                }
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
    int i,j;

    for(i=0; i<SO_USERS_NUM;i++){
        if(semop(sem_users_id, &sop_p, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }
        if(user[i].status==0)user_dead++;
        if(semop(sem_users_id, &sop_r, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }
    }
    if(user_dead==SO_USERS_NUM){
        #if DEBUG == 1
            printf("User terminati--KILL");
        #endif
        exitReason=1;
        kill(getpid(), SIGINT); 
    }
    for(i=0; i<SO_NODES_NUM;i++){
        if(nodes[i].status==0)nodes_dead++;
    }

    printf("\nNODI ATTIVI: %d\tUSER ATTIVI: %d\n", SO_NODES_NUM-nodes_dead,SO_USERS_NUM - user_dead);
    
    if(SO_USERS_NUM > MAX_PROC_PRINTABLE){
        print_min_max_budget_user();
    }else{
        stampa_utenti();
    }
    
    stampa_nodi();
    user_dead=0;
    nodes_dead=0;
}

void print_min_max_budget_user(){
    int i,j;
    user_struct* user_copy;
    user_struct tmp;
    user_copy = malloc(SO_USERS_NUM*sizeof(user_struct));

    for(i = 0; i<SO_USERS_NUM; i++){
        user_copy[i]=user[i];
    }

    for (i = 0; i < SO_USERS_NUM; ++i){
        for (j = i + 1; j < SO_USERS_NUM; ++j)
        {
            if (user_copy[i].budget > user_copy[j].budget) 
            {
                tmp =  user_copy[i];
                user_copy[i] = user_copy[j];
                user_copy[j] = tmp;
            }
        }
    }            
    
    printf("---------------%d MAX BUDGET-------------\n", MAX_PROC_PRINTABLE/2);
    for(i=SO_USERS_NUM-1; i>SO_USERS_NUM-(MAX_PROC_PRINTABLE/2)-1;i--){
        printf("User %d budget: %d\n", i, user_copy[i].budget);
    }
    printf("---------------%d MIN BUDGET-------------\n", MAX_PROC_PRINTABLE/2);
    for(i=0; i<MAX_PROC_PRINTABLE/2;i++){
        printf("User %d budget: %d\n", i, user_copy[i].budget);
    }

    free(user_copy);

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
    printf("---------------NODI---------------\n");

    for( i = 0; i < SO_NODES_NUM; i++){
        printf("node:{ pid: %d, budget: %d}\n", nodes[i].pid, nodes[i].budget);
    }
       
}

void stampa_review_finale(int exit_reason){
    int i;
    printf("\n------------INIZIO REVIEW------------\n");
    switch(exit_reason){
        case 0:
            printf("--------TEMPO SCADUTO--------\n");
            break;
        case 1:
            printf("--------USER TERMINATI--------\n");
            break;
        case 2:
            printf("--------PROGRAMMA TERMINATO DA UTENTE--------\n");
            break;
        case 3:
            printf("--------MASTERBOOK PIENO--------\n");
            break;
        default:
        break;
    }

    for(i=0; i<SO_USERS_NUM;i++){
        printf("Bilancio user %d: %d\n", user[i].pid, user[i].budget);
    }
    for(i=0; i<SO_NODES_NUM;i++){
        printf("Bilancio nodo %d: %d\n", nodes[i].pid, nodes[i].budget);
    }

    for(i=0; i<SO_USERS_NUM;i++){
        if(user[i].status==0)user_dead++;
        #if DEBUG == 1
            printf("user terminati: %d\n", user_dead);
        #endif
    }
    printf("Numero utenti terminati prematuramente: %d\n", user_dead);
    printf("Numero blocchi nel masterbook: %d\n", shd_masterbook_info->num_block);
    printf("Transazioni rimanenti nelle transaction pool:\n");
    for(i=0; i<SO_NODES_NUM;i++){
        printf("Nodo %d ha %d transazioni rimanenti\n", nodes[i].pid, nodes[i].tp_size);
    }

    remove_users();
    remove_nodes();
    remove_IPC();
    printf("----------------MASTER TERMINA----------------\n");
    exit(EXIT_SUCCESS);
}

int* get_friends(int pid_me){
       
    int* f;
    int i, r_index, flag, j;

    f = (int *)malloc(sizeof(int)*SO_NUM_FRIENDS); 
    
    for(i=0; i < SO_NUM_FRIENDS; i++){
        
        flag=0;

        do{
            /*da rivedere e togliere la printf fratm*/
            r_index = get_r_pid();
          
            if(nodes[r_index].pid == pid_me) flag = 1;

            for(j=0; j<i; j++){
                if(f[j] == r_index) flag = 1;
            }
            
        }while(flag == 1);

        f[i]=nodes[r_index].pid;
    }
    
    printf("[NODO %d] {amico 1: %d, amico2: %d}\n",pid_me,f[0], f[1]);

    return f;
}

int get_r_pid(){            
    return rand() % SO_NODES_NUM;
}