export CC="gcc"

#Variabili 
export SO_USERS_NUM=100
export SO_NODES_NUM=5
export SO_BUDGET_INIT=1000
export SO_REWARD=1
export SO_MIN_TRANS_GEN_NSEC=100000000
export SO_MAX_TRANS_GEN_NSEC=200000000
export SO_RETRY=20
export SO_TP_SIZE=10
export SO_MIN_TRANS_PROC_NSEC=10000000
export SO_MAX_TRANS_PROC_NSEC=20000000
export SO_SIM_SEC=10
export SO_NUM_FRIENDS=3
export SO_HOPS=1

master: master.o nodo user shared.h
	${CC} -o master master.o

user: user.o
	${CC} -o user user.c

master.o: master.c master.h
	${CC} -c -std=c89 -pedantic master.c

nodo.o: nodo.c nodo.h shared.h
	${CC} -c -std=c89 -pedantic nodo.c

user.o: user.c user.h shared.h
	${CC} -c -std=c89 -pedantic user.c

clean:
	rm -f *.o master nodo user

run: 
	./master
