#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include "user.h"
#include "shared.h"
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>


#define TEST_ERROR                                 \
    if (errno)                                     \
    {                                              \
        fprintf(stderr,                            \
                "%s:%d: PID=%5d: Error %d (%s)\n", \
                __FILE__,                          \
                __LINE__,                          \
                getpid(),                          \
                errno,                             \
                strerror(errno));                  \
        errno = 0;                                 \
        fflush(stderr);                            \
    }
#define SH_PARAM_ID atoi(argv[1])
#define SH_NODES_ID atoi(argv[2])
#define MASTERBOOK_ID atoi(argv[3])
#define SO_NODES_NUM atoi(argv[4])
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))

int curr_balance;
node_struct* nodes;
int index_rnode;
int index_ruser;
int main(int argc, char *argv[])
{
    curr_balance=SO_BUDGET_INIT;
    nodes = shmat(SH_NODES_ID, NULL, 0);
    TEST_ERROR;
    srand(time(NULL));
    index_rnode= rand() % SO_NODES_NUM;
    printf("index random node: %d\n", index_rnode);
    if(curr_balance<=2){
            index_ruser= rand() % SO_NODES_NUM;

    }
    printf("main user\n");
}
