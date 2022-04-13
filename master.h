/*L'uso di #pragma once può ridurre i tempi di compilazione, 
perché il compilatore non apre e legge nuovamente il file #include dopo il primo file 
nell'unità di conversione*/
#pragma once 

/*Definizione di una struct transaction*/
 struct transaction {
    int timestamp;
    int sender;
    int receiver;
    int amount;
    int reward;
};
void genera_nodi();
void genera_utenti();