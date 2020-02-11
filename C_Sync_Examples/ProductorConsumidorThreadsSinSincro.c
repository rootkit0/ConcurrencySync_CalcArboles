#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define Tamany 10

void* HiloProductor(int *Par);
void* HiloConsumidor(int *Par);
int in=0, out=0, producidas=0, totalproducidas=0, buffer[10]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

pthread_mutex_t Mutex;
pthread_cond_t  BufferLleno, BufferVacio;

main() {
	pthread_attr_t attr;
	pthread_t tid[2];
	int par,*Estado ,*Estado2;

	pthread_mutex_init(&Mutex, NULL);
	pthread_cond_init(&BufferLleno, NULL);
	pthread_cond_init(& BufferVacio, NULL);
	pthread_attr_init(&attr);
	par=1;
	if (pthread_create(&tid[0],&attr,(void*)HiloProductor,(void*)&par)!=0) {
		perror("Hilo Productor"); exit(-1);
	}
	par=-22;
	if (pthread_create(&tid[1],&attr,(void*)HiloConsumidor,(void*)&par)!=0)
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
   int error=1,x;
	
   fprintf(stdout,"Hilo %d Productor iniciado.\n",(int)pthread_self()); 
   while(error>=0)    
   {
		pthread_mutex_lock (&Mutex);  
		while (producidas==Tamany)  
		  pthread_cond_wait(&BufferVacio, &Mutex);
		pthread_mutex_unlock (&Mutex); 
		if (buffer[in]>0) 	{
		  error=-1;
		}
		else 
		{
		  //printf("P: \tPosición %d producida = %d.\n",in,buffer[in]);
		  pthread_mutex_lock (&Mutex);
		  producidas++;
		  buffer[in] = ++totalproducidas;
		  in = (in+1)%Tamany;
		  pthread_cond_broadcast(&BufferLleno);
		  if (totalproducidas%1111)
		  {
			 printf("In:%d  Out:%d  Prod:%d  Bufer:{%d",in,out,producidas,buffer[0]);
			 for (x=1;x<Tamany;x++) printf(",%d",buffer[x]);
			 printf("}\n");
		  }
		  pthread_mutex_unlock (&Mutex); 
		}
   }
   return(&error);
}


void* 
HiloConsumidor(int *Par)
{
  int error=2;
  fprintf(stdout,"Hilo %d Consumidor iniciado.\n",(int)pthread_self());
  
  while(error>=0)
  {
	 pthread_mutex_lock (&Mutex);
	 while (producidas==0)
		pthread_cond_wait(&BufferLleno, &Mutex);
	 pthread_mutex_unlock(&Mutex);
	 if (buffer[out]<=0) {
		error=-1;
	 }
	 else 
	 {
		//printf("C: \tPosición %d consumida = %d.\n",out,buffer[out]);
		pthread_mutex_lock (&Mutex);
		buffer[out] = -buffer[out];
		out = (out+1)%Tamany;
		producidas--;
		pthread_cond_broadcast(&BufferVacio);
		pthread_mutex_unlock (&Mutex);
	 }
  }
  return(&error);
}




