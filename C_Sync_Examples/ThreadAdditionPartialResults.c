#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

#define DMaxThreads 200
#define DMaxNumbers 10000000

/* Global & Shared variables */
long long int Total=0;

/* Mutex */
pthread_mutex_t Mutex;
pthread_cond_t CondPartial;
pthread_barrier_t Barrera;

int N=2, C=0; 
long int P;

struct Interval {
	long int begin;
	long int end;
};

typedef struct Interval TInterval, PtrInterval;

/* Code for the thread */
void *ThreadAddFunction(PtrInterval *i)
{
	long int n,x,s;
	long long int partial;
	
	printf("[%x] Begin Thread %ld-%ld (Parcial=%ld).\n",(unsigned int)pthread_self(), i->begin, i->end-1, P);
				
	partial=0;
	for(n=i->begin,s=1;n<i->end;n++,s++) 
	{
		partial = partial + n;	// Sección Crítica.
		if (s==P)
		{
			pthread_mutex_lock(&Mutex);
			C++;
			Total+=partial;
			partial=0;
			s=0;
			if (i->begin==1)
			{ 	
				// Hilo Bloqueado
			  	while(C<N)
			  	{
					printf("[%ld] Hilo Bloqueado esperando %d/%d condiciones para la %ld suma parcial (%ld).\n",pthread_self(), C, N, n/P, n);
			  		pthread_cond_wait(&CondPartial,&Mutex);
			  	}
			  	printf("Suma Parcial %lld.\n",Total);
			  	C=0;
			}
			else
			{ // Hilo Desbloqueado
				printf("[%ld] Hilo notificando condicion %d/%d para la %ld suma parcial (%ld).\n",pthread_self(), C, N, (n-i->begin)/P, (n-i->begin));
				pthread_cond_signal(&CondPartial);
			}
			pthread_mutex_unlock(&Mutex);
			pthread_barrier_wait(&Barrera);
		}
	}
	
	/* Entrada Seccción Crítica */
	pthread_mutex_lock(&Mutex);
	
		Total = Total + partial;	// Sección Crítica.
	
	/* Salida Seccción Crítica */
	pthread_mutex_unlock(&Mutex);
		
	printf("[%x] End Thread.\n",(unsigned int)pthread_self());
}


int main(int argc,char *argv[])
{
	struct timeval begin_time, end_time, slapsed_time;
	pthread_t thread[DMaxThreads];
	pthread_attr_t attr;
	long int M=DMaxNumbers, pendiente;

	TInterval Intervals[DMaxThreads];
	int h;

	if (argc<3)
	{
		printf("ThreadAddition [NumThreads = 2 default] [MaxNumber = 1000000 default].\n");
		exit(1);
	}

	if (argc>1)
		N=atoi(argv[1]);

	if (argc>2)
		M=atoi(argv[2]);
	
	gettimeofday(&begin_time,NULL);

	pthread_mutex_init(&Mutex,NULL);
	pthread_cond_init(&CondPartial,NULL);
	pthread_barrier_init(&Barrera,NULL,N);
	
	P=M/(N*10);
	pendiente = M;
	for (h=0;h<N;h++)
	{	/* Thread h */
		int work;

		work = pendiente/(N-h);
		if (h==0)
			Intervals[h].begin = 1;
		else
			Intervals[h].begin = Intervals[h-1].end;

		if (h==(N-1))
			Intervals[h].end = M+1;
		else
			Intervals[h].end = Intervals[h].begin + work;
		pendiente-=work;

		pthread_create(&thread[h], NULL, (void * (*)(void *))ThreadAddFunction, &(Intervals[h]));
	}
	
	for (h=0;h<N;h++)
	{
		pthread_join(thread[h],NULL);
	}
	
	pthread_mutex_destroy(&Mutex);
	pthread_cond_destroy(&CondPartial);
	pthread_barrier_destroy(&Barrera);
	
	gettimeofday(&end_time,NULL);
	
	printf("Addition Result 1 to %ld: %lld.\n", M, Total);
	
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

