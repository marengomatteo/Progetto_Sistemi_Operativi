#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "master.h"

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
    }

#define SO_USERS_NUM 2
#define SO_NODES_NUM 1
#define SO_BUDGET_INIT
#define SO_REWARD
#define SO_MIN_TRANS_GEN_NSEC
#define SO_MAX_TRANS_GEN_NSEC
#define SO_RETRY
#define SO_TP_SIZE
#define SO_BLOCK_SIZE
#define SO_MIN_TRANS_PROC_NSEC
#define SO_MAX_TRANS_PROC_NSEC
#define SO_REGISTRY_SIZE
#define SO_SIM_SEC
#define SO_FRIENDS_NUM
#define SO_HOPS

int main()
{
}

genera_nodi()
{
    int i;

    for (i = 0; i < SO_NODES_NUM; i++)
    {
        switch (fork())
        {
        case 0:
        // Passare parametri per creazioni
        // execve();
        case -1:
            TEST_ERROR;
        default:
            break;
        }
    }
}

genera_utenti()
{
    int i;

    for (i = 0; i < SO_USERS_NUM; i++)
    {
        switch (fork())
        {
        case 0:
        // Passare parametri per creazioni
        // execve();
        case -1:
            TEST_ERROR;
        default:
            break;
        }
    }
}