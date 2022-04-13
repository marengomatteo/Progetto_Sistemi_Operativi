#pragma once 

 typedef struct {
    int timestamp;
    int sender;
    int receiver;
    int amount;
    int reward;
} transaction;

typedef struct _node {
  transaction transaction;
  struct _node *next;
} node;

typedef node *transaction_pool;

// transaction_pool l_add_at(int i, transaction d, transaction_pool l){
// // cond: i in [0,n] dove n è la dimensione della lista l
// 	if (l==NULL){ // lista vuota: ammesso solo inserimento in posizione 0
// 	   if (i==0){	
// 		   printf("adding at the head\n");
// 	      node *el = (node *)malloc(sizeof(node));
// 	      el->d = d;
// 	      el->next=NULL;
// 	      l=el;
// 	   }
// 	}else{ // la lista l ha almeno un elemento
// 	   node *el = (node *)malloc(sizeof(node));
// 	   el->d = d;
// 	   if (i==0){ // caso limite: inserisco in testa (posizione 0)
// 	      el->next=l;
// 	      l = el;
// 	   }else{
//               node *p=l;
// 	      int j = 1;
// 	       // p punta al nodo alla posizione j-1
// 	      while(p!=NULL && j!=i){
// 		    p=p->next;
//           	    j++;
// 	      }
// 	      if (p!=NULL){
// 	            node *t=p->next;
//                     p->next = el;
// 		    el->next = t;
// 	      }   

// 	   }
// 	}
// 	return l;
// }

// void l_print(list l){
// 	printf("(");
// 	for ( ; l!=NULL ; l=l->next)
// 	   printf("%d, ",*(l->d));
// 	printf("\b\b)\n");
// }

