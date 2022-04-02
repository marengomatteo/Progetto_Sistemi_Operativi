export CC="gcc"
export CFLAGS= "-std=c89 -pedantic"

export SO_USERS_NUM = 2
export SO_NODES_NUM = 2
export SO_BUDGET_INIT = 100
export SO_REWARD = 1
export SO_MIN_TRANS_GEN_NSEC = 1000000000
export SO_MAX_TRANS_GEN_NSEC = 2000000000
export SO_RETRY = 20
export SO_TP_SIZE = 1000
export SO_MIN_TRANS_PROC_NSEC = 100000000
export SO_MAX_TRANS_PROC_NSEC = 2000000000
export SO_SIM_SEC = 10
export SO_FRIENDS_NUM = 3
export SO_HOPS = 10
#Variabili 

master: master.o 
	gcc -o master *.o

master.o: master.c master.h
	gcc -c master.c

clean:
	rm -f *.o

run:
	./master 