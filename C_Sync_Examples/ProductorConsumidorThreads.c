#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define Tamany 100
#define DATOS_A_PRODUCIR 100000

void* HiloProductor(int *Par);
void* HiloConsumidor(int *Par);
int in=0, out=0, producidas=0, totalproducidas=0, totalconsumidas=0, buffer[Tamany];

pthread_mutex_t Mutex;
pthread_cond_t  BufferLleno, BufferVacio;

main() {
	pthread_t tid[2];
	int par[2],*Estado ,*Estado2, x;

	for(x=0;x<Tamany;x++) 
		buffer[x]=0;
	
	pthread_mutex_init(&Mutex, NULL);
	pthread_cond_init(&BufferLleno, NULL);
	pthread_cond_init(& BufferVacio, NULL);
	par[0]=1;
	if (pthread_create(&tid[0], NULL, (void*)HiloProductor, (void*)&(par[0]))!=0) {
		perror("Hilo Productor"); exit(-1);
	}
	par[1]=-1;
	if (pthread_create(&tid[1], NULL, (void*)HiloConsumidor,(void*)&(par[1]))!=0)
	{
		perror("Hilo Consumidor");
		exit(-1);
	}

	pthread_join(tid[0],(void**)&Estado);
	pthread_join(tid[1],(void**)&Estado2);
	printf("Estado Hilo Productor %d.\n",*Estado);
	printf("Estado Hilo Consumidor %d.\n",*Estado2);

	pthread_mutex_destroy(&Mutex);
	pthread_cond_destroy(&BufferLleno);
	pthread_cond_destroy(& BufferVacio);
	
	exit(0);
}


void* 
HiloProductor(int *Par) 
{
   int *error,x;
	
	error = malloc(sizeof(int));
	*error = 1;
	
   printf("Hilo %x Productor iniciado.\n",(unsigned int)pthread_self()); 
   while(*error>=0 && totalproducidas<DATOS_A_PRODUCIR)    
   {
		pthread_mutex_lock (&Mutex);  
		while (producidas==Tamany)  
		  pthread_cond_wait(&BufferVacio, &Mutex);
		if (buffer[in]>0) 	{
			*error=-1;
			pthread_mutex_unlock (&Mutex); 
		}
		else 
		{
		  producidas++;
		  buffer[in] = ++totalproducidas;
		  in = (in+1)%Tamany;
		  pthread_mutex_unlock (&Mutex); 
		  pthread_cond_signal(&BufferLleno);
		  if ((totalproducidas%1000)==0)
		  {
			 printf("%d -> In:%d  Out:%d  Prod:%d.\n",totalproducidas,in,out,producidas);
			 //for (x=1;x<Tamany;x++) printf(",%d",buffer[x]);
			 //printf("}\n");
		  }
		}
   }
   return(error);
}


void* 
HiloConsumidor(int *Par)
{
  int *error;

  
  fprintf(stdout,"Hilo %d Consumidor iniciado.\n",(int)pthread_self());
  
  error = malloc(sizeof(int));
  *error = 2;
	
  while(*error>=0 && totalconsumidas<DATOS_A_PRODUCIR)
  {
	 pthread_mutex_lock (&Mutex);
	 while (producidas==0)
		pthread_cond_wait(&BufferLleno, &Mutex);
	 if (buffer[out]<=0) {
		*error=-1;
	   pthread_mutex_unlock(&Mutex);
	 }
	 else 
	 {
		buffer[out] = -buffer[out];
		out = (out+1)%Tamany;
		producidas--;
		totalconsumidas++;
		pthread_cond_signal(&BufferVacio);
		pthread_mutex_unlock (&Mutex);
	 }
  }
  return(error);
}




