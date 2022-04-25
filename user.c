#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

/*#define SH_PARAM_ID atoi(argv[1])
#define SO_BUDGET_INIT atoi(getenv("SO_BUDGET_INIT"))*/

int main(int argc, char *argv[])
{
    printf("main user\n");
}
