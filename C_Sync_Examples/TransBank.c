#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define DMaxAccounts 25
#define DMaxThreads 100
#define DConcTransactions 10
#define DTransactions 100
#define DMaxTransactions 10000
#define DMaxAmount 100

/* Global & Shared variables */
long int Cuentas[DMaxAccounts];
sem_t SemConcTrans, SemMutex;

long int M=DTransactions;


typedef enum {EIngreso, EReintegro} ETransType;

struct Transaction {
	int 		account;
	ETransType 	type;
	int 		amount;
};

typedef struct Transaction TTransaction, *PtrTransaction;


void *ClientThread(TTransaction Transactions[]);
// Operaciones con Transacciones.
void DecrementarSaldo(int account, int amount);
void IncrementarSaldo(int account, int amount);


int main(int argc,char *argv[])
{
	struct timeval begin_time, end_time, slapsed_time;
	pthread_t thread[DMaxThreads];
	pthread_attr_t attr;
	int N=2;
	TTransaction Transacciones[DMaxThreads][DMaxTransactions];
	int h, t, c;

	if (argc<3)
	{
		printf("TransBank [NumThreads = 2 default] [MaxTransactions = 100 default].\n");
		exit(1);
	}

	if (argc>1)
		N=atoi(argv[1]);

	if (argc>2)
		M=atoi(argv[2]);

	gettimeofday(&begin_time,NULL);
	
	sem_init(&SemMutex, 0, 1);
	sem_init(&SemConcTrans, 0, DConcTransactions);

	for (h=0;h<N;h++)
	{	/* Thread h */

		for(t=0;t<M;t++) 
		{
			Transacciones[h][t].account = (rand()/RAND_MAX)* DMaxAccounts;
			Transacciones[h][t].type = (rand()/RAND_MAX)* (EReintegro+1);
			Transacciones[h][t].amount = ((rand()/RAND_MAX)* DMaxAmount ) * 100;
		}
		pthread_create(&thread[h], NULL, (void * (*)(void *))ClientThread, &(Transacciones[h]));
	}
	
	for (h=0;h<N;h++)
	{
		pthread_join(thread[h],NULL);
	}
	
	gettimeofday(&end_time,NULL);
	
	for (c=0;c<DMaxAccounts;c++)
	{
		printf("Account %d Balance: %ld.\n", c, Cuentas[c]);
	}
	
	// Calculated execution time.
	slapsed_time.tv_sec = end_time.tv_sec - begin_time.tv_sec;
	slapsed_time.tv_usec = end_time.tv_usec - begin_time.tv_usec;
	if (slapsed_time.tv_usec<0)
	{
		slapsed_time.tv_sec--;
		slapsed_time.tv_usec += 1000000;
	}
	printf("Time Required: %lf seconds.\n\n", (double)slapsed_time.tv_sec + ((double)slapsed_time.tv_usec/1000000));
	
	exit(0);
}


/* Code for the thread */
void *ClientThread(TTransaction Transactions[])
{
	struct timeval begin_time, end_time, slapsed_time;
	int t;
	
	printf("[%x] Begin Thread.\n",(unsigned int)pthread_self());
				
	for(t=0;t<M;t++) 
	{
		/* Sincronización Condicional */
		sem_wait(&SemConcTrans);
		
		switch(Transactions[t].type)
		{
			case EIngreso:
				IncrementarSaldo(Transactions[t].account, Transactions[t].amount);
				break;
				
			case EReintegro:
				DecrementarSaldo(Transactions[t].account, Transactions[t].amount);
				break;		
		}
		
		/* Sincronización Condicional */
		sem_post(&SemConcTrans);
	}
	printf("[%x] End Thread.\n",(unsigned int)pthread_self());
}


// Operaciones con Transacciones.
void DecrementarSaldo(int account, int amount)
{
	sem_wait(&SemMutex);
	Cuentas[account] -= amount;		/* Sección Crítica */
	sem_post(&SemMutex);
}

void IncrementarSaldo(int account, int amount)
{
	sem_wait(&SemMutex);
	Cuentas[account] += amount;		/* Sección Crítica */
	sem_post(&SemMutex);
}


