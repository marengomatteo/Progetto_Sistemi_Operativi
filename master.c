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
int sem_nodes_id;
int sem_nodes_users_id;
int sem_masterbook_id;

struct sembuf sops;
struct sembuf sop_p;
struct sembuf sop_r;

/* Array per tener traccia delle risorse create SHAREDMEMORY */
int shared_nodes_id; /* id memoria condivisa dei nodi*/
int shared_masterbook_id; /* id mlibro mastro*/
int shared_users_id; /* id memoria condivisa degli user*/

char *node_arguments[7] = {NODE_NAME};
char *user_arguments[8] = {USER_NAME};

struct timespec timestamp;
node_struct *nodes;
user_struct *user;
masterbook_struct* shd_masterbook_info;
int shared_masterbook_info_id;
int id_queue_friends;
int id_queue_pid_friends;
/* PARTE DA 24
int id_queue_message_rejected;*/
int num_max_nodes;
int exitReason=2, user_dead=0;
message_id_f *msg_id_friends;

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
        case SIGUSR2:
            exitReason = 4;
            break;
        default:
        break;
   }
   stampa_review_finale(exitReason);
}

int main(int argc, char **argv, char **envp){

    int i;
    int r_index;
    message_f *msg_friend;
    message msg;
    char id_argument_sm_nodes[3 * sizeof(int) + 1]; /*id memoria condivisa nodi*/
    char id_argument_sm_users[3 * sizeof(int) + 1]; /*id memoria condivisa user*/
    char id_argument_sm_masterbook[3 * sizeof(int) + 1]; /*id memoria condivisa master book*/
    char id_argument_sm_masterinfo[3 * sizeof(int) + 1]; /*id memoria condivisa masterbook*/

    char id_argument_sem_id[3 * sizeof(int) + 1]; /*id semaforo user e nodi*/
    char id_argument_sem_users_id[3*sizeof(int)+1]; /*id semaforo per user*/
    char id_argument_sem_nodes_id[3*sizeof(int)+1]; /*id semaforo per nodi*/
    char id_argument_sem_masterbook_id[3 * sizeof(int) + 1]; /*id semaforo per scrivere e leggere su masterbook*/

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
    if (signal(SIGUSR2, sig_handler)==SIG_ERR) {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }

    if(SO_TP_SIZE <= SO_BLOCK_SIZE) {
        printf("Parametri errati inserire SO_TP_SIZE maggiore di SO_BLOCK_SIZE\n");
        exit(EXIT_FAILURE);
    }
   
    alarm(SO_SIM_SEC);

    msg_friend = (message_f*)malloc(sizeof(message_f));
    msg_id_friends = (message_id_f*)malloc(sizeof(message_id_f));

    num_max_nodes= SO_NODES_NUM*2;

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
    sprintf(id_argument_sem_nodes_id,"%d",sem_nodes_id);

    /* Argomenti per nodo*/
    node_arguments[0] = id_argument_sm_nodes;
    node_arguments[2] = id_argument_sm_masterbook;
    node_arguments[3] = id_argument_sm_masterinfo;
    node_arguments[4] = id_argument_sem_masterbook_id;
    node_arguments[5] = id_argument_sem_id;
    node_arguments[6] = id_argument_sem_nodes_id;

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

    genera_nodi(envp, SO_NODES_NUM);
    genera_utenti(envp);

    
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(sem_nodes_users_id, &sops, 1);

    for(i=0; i<SO_NODES_NUM;i++)
        get_friends(i);
    
    timestamp.tv_sec = 1;
    timestamp.tv_nsec = 0;
    
    while(1){
        /* controllo se ci sono messaggi dei nodi per il master */

        if(msgrcv(id_queue_friends, msg_friend, sizeof(message_f), getpid(),IPC_NOWAIT)>0 && shd_masterbook_info->num_nodes<num_max_nodes){
            printf("master riceve\n");
            /* creo nuovo nodo*/
            semctl(sem_nodes_users_id,0,SETVAL,1);
            genera_nodi(envp, 1);

            sops.sem_op=0;
            sops.sem_num=0;
            semop(sem_nodes_users_id,&sops,1);
            
            get_friends(shd_masterbook_info->num_nodes-1);

            msg.mtype = nodes[shd_masterbook_info->num_nodes-1].pid;
            msg.trans = msg_friend->trans;

            /* Invio la transazione arrivata al nodo appena creato*/
            if(msgsnd(nodes[shd_masterbook_info->num_nodes-1].id_mq,&msg,sizeof(message),0) < 0){
                perror("errore nella message send della transazione da mastera a nodo linea 191\n");
                return -1;
            }
            
            /*
                Invio tutte le transazioni ricevute al nodo appena creato
                Quando il master uscirà dal while non avrà più messaggi nella sua coda,
                in un secondo momento quando arriveranno altre transazioni il master 
                creerà un altro nodo.
            */
            while(msgrcv(id_queue_friends, msg_friend, sizeof(message_f), getpid(),IPC_NOWAIT)>0){
                msg.mtype = nodes[shd_masterbook_info->num_nodes-1].pid;
                msg.trans = msg_friend->trans;
                if(msgsnd(nodes[shd_masterbook_info->num_nodes-1].id_mq,&msg,sizeof(message),0) < 0){
                    perror("errore nella message send della transazione da mastera a nodo linea 191\n");
                    return -1;
                }
            }

            for(i=0; i<SO_NUM_FRIENDS;i++){
                srand(clock());
                r_index = rand() % (shd_masterbook_info->num_nodes-1);
                msg_id_friends->friend=nodes[shd_masterbook_info->num_nodes-1].pid;
                msg_id_friends->mtype=nodes[r_index].pid;
                if(msgsnd(id_queue_pid_friends, msg_id_friends, sizeof(message_id_f),0)<0){
                    perror("errore message invio amici send linea 206");
                    return -1;
                }
            }
        }
        if(shd_masterbook_info->num_nodes == num_max_nodes){
            printf("Numero nodi massimo raggiunto\n");
            kill(getpid(), SIGUSR2);
        };
        stampa_info();
        nanosleep(&timestamp, NULL);
    }
    
    return 0;  
}

int ipc_init(){

    /*Creo coda di messaggi amici*/
    id_queue_friends = msgget(ID_QUEUE_FRIENDS,IPC_CREAT | 0600);

    /*Creo coda di messaggi pid amici*/
    id_queue_pid_friends = msgget(ID_QUEUE_FRIENDS_PID,IPC_CREAT | 0600);

    /* PARTE DA 24 Coda di messaggi per transazioni rifiutate 
    id_queue_message_rejected = msgget(ID_QUEUE_MESSAGE_REJECTED, IPC_CREAT | 0600);*/

    /* Create a shared memory area for nodes struct */
    shared_nodes_id = shmget(IPC_PRIVATE, num_max_nodes * sizeof(node_struct), 0600);
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
    shd_masterbook_info->num_nodes = 0;

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
    sem_nodes_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if(sem_nodes_id == -1){
        perror("Errore durante la creazione del semaforo per accedere alle informazioni dei nodi\n");
        return -1;
    }
}

void genera_nodi(char **envp, int num_nodes)
{
    char node_id[3 * sizeof(int) + 1];
    int i,current_nodes;
    int msgq_id;
    current_nodes = shd_masterbook_info->num_nodes;

    for (i = shd_masterbook_info->num_nodes; i < current_nodes+num_nodes; i++)
    {
        switch (fork())
        {
            case 0:
                               
                sprintf(node_id, "%d", i);
                node_arguments[1] = node_id;

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

                /* INSTANZIARE CON EXECVE IL NODO, Passare parametri */
                if (execve(NODE_NAME, node_arguments, envp) == -1){
                    perror("Could not execve");
                    exit(EXIT_FAILURE);
                }
            case -1:
                exit(EXIT_FAILURE);
            default:             
                shd_masterbook_info->num_nodes++;
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

                if(semop(sem_nodes_users_id, &sop_p, 1)<0){
                    printf("Errore semaforo sem nodes users\n");
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
    semctl(sem_nodes_id, 0, SETVAL, 1);

}

void remove_IPC(){
    int i;
    for (i=0;i<shd_masterbook_info->num_nodes ;i++)
    {
        msgctl(nodes[i].id_mq, IPC_RMID, NULL);
    }
    msgctl(id_queue_friends, IPC_RMID, NULL);
    msgctl(id_queue_pid_friends, IPC_RMID, NULL);
    /* PARTE DA 24
    msgctl(id_queue_message_rejected, IPC_RMID, NULL);*/
    semctl(sem_nodes_users_id,0, IPC_RMID);
    semctl(sem_users_id, 0, IPC_RMID);
    semctl(sem_masterbook_id, 0, IPC_RMID);
    semctl(shd_masterbook_info->sem_masterbook, 0, IPC_RMID);
    semctl(sem_nodes_id, 0, IPC_RMID);
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
    for( i = 0; i < shd_masterbook_info->num_nodes; i++){
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

    printf("\nNODI ATTIVI: %d\tUSER ATTIVI: %d\n", shd_masterbook_info->num_nodes,SO_USERS_NUM - user_dead);
    
    if(SO_USERS_NUM > MAX_PROC_PRINTABLE){
        print_min_max_budget_user();
    }else{
        stampa_utenti();
    }
    
    stampa_nodi();
    user_dead=0;
}

void print_min_max_budget_user(){
    int i,j;
    user_struct* user_copy;
    user_struct tmp;
    user_copy = malloc(SO_USERS_NUM*sizeof(user_struct));

    for(i = 0; i < SO_USERS_NUM; i++){
        if(semop(sem_users_id, &sop_p, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }

        user_copy[i]=user[i];

        if(semop(sem_users_id, &sop_r, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }
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
        printf("User %d budget: %d\n", user_copy[i].pid, user_copy[i].budget);
    }
    printf("---------------%d MIN BUDGET-------------\n", MAX_PROC_PRINTABLE/2);
    for(i=0; i<MAX_PROC_PRINTABLE/2;i++){
        printf("User %d budget: %d\n", user_copy[i].pid, user_copy[i].budget);
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

    for( i = 0; i < shd_masterbook_info->num_nodes; i++){
        if(semop(sem_nodes_id, &sop_p, 1) == -1){
            perror("errore nel semaforo dei nodi preso\n");
            exit(EXIT_FAILURE);
        }
       
        printf("node:{ pid: %d, budget: %d}\n", nodes[i].pid, nodes[i].budget);

        if(semop(sem_nodes_id, &sop_r, 1) == -1){
            perror("errore nel semaforo dei nodi rilascio\n");
            exit(EXIT_FAILURE);
        }

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
        case 4:
            printf("--------RAGGIUNTO NUMERO MASSIMO DI NODI--------\n");
            break;
        default:
        break;
    }

    for(i=0; i<SO_USERS_NUM;i++){

        if(semop(sem_users_id, &sop_p, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }
        printf("Bilancio user %d: %d\n", user[i].pid, user[i].budget);

        if(semop(sem_users_id, &sop_r, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }
    }

    for(i=0; i<shd_masterbook_info->num_nodes;i++){

        if(semop(sem_nodes_id, &sop_p, 1) == -1){
            perror("errore nel semaforo dei nodi preso\n");
            exit(EXIT_FAILURE);
        }
       
        printf("Bilancio nodo %d: %d\n", nodes[i].pid, nodes[i].budget);

        if(semop(sem_nodes_id, &sop_r, 1) == -1){
            perror("errore nel semaforo dei nodi rilascio\n");
            exit(EXIT_FAILURE);
        }
    }

    for(i=0; i<SO_USERS_NUM;i++){

        if(semop(sem_users_id, &sop_p, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }
        if(user[i].status==0) user_dead++;

        if(semop(sem_users_id, &sop_r, 1) == -1){
            perror("errore nel semaforo preso per lettura user da master\n");
        }        
        #if DEBUG == 1
            printf("user terminati: %d\n", user_dead);
        #endif
    }
    printf("Numero utenti terminati prematuramente: %d\n", user_dead);

    if(semop(sem_masterbook_id, &sop_p, 1) == -1){
        perror("errore nel semaforo preso per lettura user da master\n");
    }
    printf("Numero blocchi nel masterbook: %d\n", shd_masterbook_info->num_block);

    if(semop(sem_masterbook_id, &sop_r, 1) == -1){
        perror("errore nel semaforo preso per lettura user da master\n");
    }        
    printf("Transazioni rimanenti nelle transaction pool:\n");
    for(i=0; i<shd_masterbook_info->num_nodes;i++){

        if(semop(sem_nodes_id, &sop_p, 1) == -1){
            perror("errore nel semaforo dei nodi preso\n");
            exit(EXIT_FAILURE);
        }
       
        printf("Nodo %d ha %d transazioni rimanenti\n", nodes[i].pid, nodes[i].tp_size);

        if(semop(sem_nodes_id, &sop_r, 1) == -1){
            perror("errore nel semaforo dei nodi rilascio\n");
            exit(EXIT_FAILURE);
        }
    }

    remove_users();
    remove_nodes();
    remove_IPC();
    printf("----------------MASTER TERMINA----------------\n");
    exit(EXIT_SUCCESS);
}

int get_r_pid(){
    srand(clock());
    return rand() % shd_masterbook_info->num_nodes;
}

void get_friends(int node_id){
       
    int* f;
    int i, r_index, flag, j;
    
    
    f = (int*) malloc(sizeof(int)*SO_NUM_FRIENDS); 
    
    for(i=0; i < SO_NUM_FRIENDS; i++){
        do{
            flag=0;
            r_index = get_r_pid();
            if(nodes[r_index].pid == nodes[node_id].pid) flag = 1;

            for(j=0; j<i && flag==0; j++){
                if(f[j] == nodes[r_index].pid) flag = 1;
            }
            
        }while(flag == 1);
        f[i]=nodes[r_index].pid;
        msg_id_friends->mtype=nodes[node_id].pid;
        msg_id_friends->friend=f[i];
        /* printf("nodo %d creato da master: %d\n", node_id,nodes[node_id].pid);*/ 
        if(msgsnd(id_queue_pid_friends,msg_id_friends,sizeof(message_id_f),0) < 0){
            perror("errore nella message send ad un amico master\n");
        }
        
    }
    free(f);
}