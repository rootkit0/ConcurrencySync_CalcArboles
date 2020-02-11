#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define DMaxThreads 200
#define DMaxNumbers 10000000

/* Global & Shared variables */
long long int Total=0;

/* Mutex */
pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;

struct Interval {
	long int begin;
	long int end;
};

typedef struct Interval TInterval, PtrInterval;

/* Code for the thread */
void *ThreadAddFunction(PtrInterval *i)
{
	long int n,x;
	
	printf("[%x] Begin Thread %ld-%ld.\n",(unsigned int)pthread_self(), i->begin, i->end);
				
	for(n=i->begin;n<=i->end;n++) 
	{
		/* Entrada Seccción Crítica */
		pthread_mutex_lock(&Mutex);
		
			Total = Total + n;	// Sección Crítica.
		
		/* Salida Seccción Crítica */
		pthread_mutex_unlock(&Mutex);
	}
	printf("[%x] End Thread.\n",(unsigned int)pthread_self());
}


int main(int argc,char *argv[])
{
	struct timeval begin_time, end_time, slapsed_time;
	pthread_t thread[DMaxThreads];
	pthread_attr_t attr;
	int N=2;
	long int M=DMaxNumbers;
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
	
	for (h=0;h<N;h++)
	{	/* Thread h */

		if (h==0)
			Intervals[h].begin = 1;
		else
			Intervals[h].begin = Intervals[h-1].end+1;

		if (h==(N-1))
			Intervals[h].end = M;
		else
			Intervals[h].end = Intervals[h].begin + ((M-Intervals[h-1].end)-1)/(N-h);

		pthread_create(&thread[h], NULL, (void * (*)(void *))ThreadAddFunction, &(Intervals[h]));
	}
	
	for (h=0;h<N;h++)
	{
		pthread_join(thread[h],NULL);
	}
	
	pthread_mutex_destroy(&Mutex);
	
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

