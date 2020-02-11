/* ---------------------------------------------------------------
Práctica 3.
Código fuente: CalcArbolesSync.c
Grau Informàtica
48257114D Xavier Berga Puig.
21020761A Jose Almunia Bourgon.
---------------------------------------------------------------
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <time.h>

#include "ConvexHull.h"

#include <pthread.h>
#include <semaphore.h>

#define DMaxArboles 	25
#define DMaximoCoste 999999
#define S 10000
#define DDebug 0
#define M 25000

  //////////////////////////
 // Estructuras de datos //
//////////////////////////

// Definicion estructura arbol entrada (Conjunto arboles).
struct  Arbol
{
	int	  IdArbol;
	Point Coord;			// Posicion bool
	int Valor;				// Valor / Coste bool.
	int Longitud;			// Cantidad madera bool
};
typedef struct Arbol TArbol, *PtrArbol;

// Definicion estructura bosque entrada (Conjunto arboles).
struct Bosque
{
	int 		NumArboles;
	TArbol 	Arboles[DMaxArboles];
};
typedef struct Bosque TBosque, *PtrBosque;

// Combinacion.
struct ListaArboles
{
	int 		NumArboles;
 	float		Coste;
	float		CosteArbolesCortados;
	float		CosteArbolesRestantes;
	float		LongitudCerca;
	float		MaderaSobrante;
	int 		Arboles[DMaxArboles];
};
typedef struct ListaArboles TListaArboles, *PtrListaArboles;

// Vector estatico coordenadas.
typedef Point TVectorCoordenadas[DMaxArboles], *PtrVectorCoordenadas;

typedef enum {false, true} bool;

  ////////////////////////
 // Variables Globales //
////////////////////////

TBosque ArbolesEntrada;
pthread_t *threads;
long int num_threads;
struct Rango {
	int index;
	long int combinacion_inicial;
	long int combinacion_final;
	int Coste;
	long int CosteMejorCombinacion;
	long int MejorCombinacion;
	//TListaArboles OptimoParcial;
};
struct Rango *RangoCombinaciones;
clock_t start, end, final;

TListaArboles OptimoParcialGlobal;
pthread_mutex_t mutex;
pthread_barrier_t barrier;
sem_t semaforo;

//Estadisticas
struct Estadistica {
	int EstCombinaciones, EstCombValidas, EstCombInvalidas;
	long EstCostePromedio;
	int EstMejorCosteCombinacion, EstPeorCosteCombinacion, EstMejorArboles, EstMejorArbolesCombinacion, EstPeorArboles, EstPeorArbolesCombinacion;
	float EstMejorCoste, EstPeorCoste;
};
struct Estadistica *Estadisticas;
struct Estadistica EstadisticasGlobales;
int EstadisticasParciales;

  //////////////////////////
 // Prototipos funciones //
//////////////////////////

bool LeerFicheroEntrada(char *PathFicIn);
bool GenerarFicheroSalida(TListaArboles optimo, char *PathFicOut);
bool CalcularCercaOptima(PtrListaArboles Optimo);
void OrdenarArboles();
bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo);
int EvaluarCombinacionListaArboles(int Combinacion, struct Estadistica *Estadisticas);
int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles);
int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados);
void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas);
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca);
float CalcularDistancia(int x1, int y1, int x2, int y2);
int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles);
int CalcularCosteCombinacion(TListaArboles CombinacionArboles);
void MostrarArboles(TListaArboles CombinacionArboles);
//Funciones auxiliares
void CombinacionOptimaThread(struct Rango *RangoCombinaciones);
void ExceptionHandler();
void InicializarEstadisticas();
void MostrarEstadisticas();

int main(int argc, char *argv[])
{
	start = clock();
	signal(SIGINT, ExceptionHandler);
	TListaArboles Optimo;

	if (argc<4 || argc>5)
		printf("Error Argumentos. Usage: CalcArboles <Fichero_Entrada> <Max_Threads> <M> [<Fichero_Salida>]");

	if (!LeerFicheroEntrada(argv[1]))
	{
		printf("Error lectura fichero entrada.\n");
		exit(1);
	}

	//leer numero de threads
    num_threads = atoi(argv[2]);
	//reservar memoria para los threads
	threads = malloc (sizeof(pthread_t)*num_threads);
	if (threads==NULL)  {
		perror("Error reserva memoria threads."); 
		exit(1);
	}
	
	//inicializar estadisticas parciales
	EstadisticasParciales = M;
	if(argc > 4) {
		EstadisticasParciales = atoi(argv[3]);
	}
	
	if (!CalcularCercaOptima(&Optimo))
	{
		printf("Error CalcularCercaOptima.\n");
		exit(1);
	}

	if (argc==4)
	{
		if (!GenerarFicheroSalida(Optimo, "./Valla.res"))
		{
			printf("Error GenerarFicheroSalida.\n");
			exit(1);
		}
	}
	else
	{
		if (!GenerarFicheroSalida(Optimo, argv[4]))
		{
			printf("Error GenerarFicheroSalida.\n");
			exit(1);
		}
	}
	end = clock();
	printf("Tiempo de ejecucion: %ld\n", end-start);
	exit(0);
}

bool LeerFicheroEntrada(char *PathFicIn)
{
	FILE *FicIn;
	int a;

	FicIn=fopen(PathFicIn,"r");
	if (FicIn==NULL)
	{
		perror("Lectura Fichero entrada.");
		return false;
	}
	printf("Datos Entrada:\n");

	// Leemos el numero de arboles del bosque de entrada.
	if (fscanf(FicIn, "%d", &(ArbolesEntrada.NumArboles))<1)
	{
		perror("Lectura arboles entrada");
		return false;
	}
	printf("\tÁrboles: %d.\n",ArbolesEntrada.NumArboles);

	// Leer atributos arboles.
	for(a=0;a<ArbolesEntrada.NumArboles;a++)
	{
		ArbolesEntrada.Arboles[a].IdArbol=a+1;
		// Leer x, y, Coste, Longitud.
		if (fscanf(FicIn, "%d %d %d %d",&(ArbolesEntrada.Arboles[a].Coord.x), &(ArbolesEntrada.Arboles[a].Coord.y), &(ArbolesEntrada.Arboles[a].Valor), &(ArbolesEntrada.Arboles[a].Longitud))<4)
		{
			perror("Lectura datos arbol.");
			return false;
		}
		printf("\tÁrbol %d-> (%d,%d) Coste:%d, Long:%d.\n",a+1,ArbolesEntrada.Arboles[a].Coord.x, ArbolesEntrada.Arboles[a].Coord.y, ArbolesEntrada.Arboles[a].Valor, ArbolesEntrada.Arboles[a].Longitud);
	}

	return true;
}

bool GenerarFicheroSalida(TListaArboles Optimo, char *PathFicOut)
{
	FILE *FicOut;
	int a;

	FicOut=fopen(PathFicOut,"w+");
	if (FicOut==NULL)
	{
		perror("Escritura fichero salida.");
		return false;
	}

	// Escribir arboles a talar.
	// Escribimos numero de arboles a talar.
	if (fprintf(FicOut, "Cortar %d árbol/es: ", Optimo.NumArboles)<1)
	{
		perror("Escribir nmero de arboles a talar");
		return false;
	}

	for(a=0;a<Optimo.NumArboles;a++)
	{
		// Escribir numero arbol.
		if (fprintf(FicOut, "%d ",ArbolesEntrada.Arboles[Optimo.Arboles[a]].IdArbol)<1)
		{
			perror("Escritura nmero �bol.");
			return false;
		}
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nMadera Sobrante: \t%4.2f (%4.2f)", Optimo.MaderaSobrante,  Optimo.LongitudCerca)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles cortados: \t%4.2f.", Optimo.CosteArbolesCortados)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles restantes: \t%4.2f\n", Optimo.CosteArbolesRestantes)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	return true;
}

bool CalcularCercaOptima(PtrListaArboles Optimo)
{
	int MaxCombinaciones;

	/* Calculo máximo combinaciones */
	MaxCombinaciones = (int) pow(2.0,ArbolesEntrada.NumArboles);

	// Ordenar arboles por segun coordenadas crecientes de x,y
	OrdenarArboles();

	/* Calculo optimo */
	Optimo->NumArboles = 0;
	Optimo->Coste = DMaximoCoste;
	CalcularCombinacionOptima(1, MaxCombinaciones, Optimo);

	return true;
}

void OrdenarArboles()
{
  int a,b;

	for(a=0; a<(ArbolesEntrada.NumArboles-1); a++)
	{
		for(b=1; b<ArbolesEntrada.NumArboles; b++)
		{
			if ( ArbolesEntrada.Arboles[b].Coord.x < ArbolesEntrada.Arboles[a].Coord.x ||
				 (ArbolesEntrada.Arboles[b].Coord.x == ArbolesEntrada.Arboles[a].Coord.x && ArbolesEntrada.Arboles[b].Coord.y < ArbolesEntrada.Arboles[a].Coord.y) )
			{
				TArbol aux;

				// aux=a
				aux.Coord.x = ArbolesEntrada.Arboles[a].Coord.x;
				aux.Coord.y = ArbolesEntrada.Arboles[a].Coord.y;
				aux.IdArbol = ArbolesEntrada.Arboles[a].IdArbol;
				aux.Valor = ArbolesEntrada.Arboles[a].Valor;
				aux.Longitud = ArbolesEntrada.Arboles[a].Longitud;

				// a=b
				ArbolesEntrada.Arboles[a].Coord.x = ArbolesEntrada.Arboles[b].Coord.x;
				ArbolesEntrada.Arboles[a].Coord.y = ArbolesEntrada.Arboles[b].Coord.y;
				ArbolesEntrada.Arboles[a].IdArbol = ArbolesEntrada.Arboles[b].IdArbol;
				ArbolesEntrada.Arboles[a].Valor = ArbolesEntrada.Arboles[b].Valor;
				ArbolesEntrada.Arboles[a].Longitud = ArbolesEntrada.Arboles[b].Longitud;

				// b=aux
				ArbolesEntrada.Arboles[b].Coord.x = aux.Coord.x;
				ArbolesEntrada.Arboles[b].Coord.y = aux.Coord.y;
				ArbolesEntrada.Arboles[b].IdArbol = aux.IdArbol;
				ArbolesEntrada.Arboles[b].Valor = aux.Valor;
				ArbolesEntrada.Arboles[b].Longitud = aux.Longitud;
			}
		}
	}
}

// Calcula la combinacion optima entre el rango de combinaciones PrimeraCombinacion-UltimaCombinacion.
bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo)
{
	long int MejorCombinacion=0, CosteMejorCombinacion;
	int Coste;

	TListaArboles CombinacionArboles;
	TVectorCoordenadas CoordArboles, CercaArboles;
	int NumArboles, PuntosCerca;
	float MaderaArbolesTalados;

	int CombinacionesPorThread = ((UltimaCombinacion - PrimeraCombinacion)/num_threads)+1;
	int CombinacionesAux = 0;
	//reservar memoria
	RangoCombinaciones = malloc(sizeof(struct Rango)*num_threads);
	Estadisticas = malloc(sizeof(struct Estadistica)*num_threads);
	if (RangoCombinaciones==NULL || Estadisticas==NULL) {
		perror("Error reserva memoria."); 
		exit(1);
	}

  	printf("Evaluacion combinaciones posibles: \n");
	CosteMejorCombinacion = Optimo->Coste;

	//inicializamos el mutex
	pthread_mutex_init(&mutex, NULL);
	//inicializamos la barrera
	int barrier_init = pthread_barrier_init(&barrier, NULL, num_threads + 1);
	if(barrier_init) {
            fprintf(stderr, "pthread_barrier_init: %d\n", strerror(barrier_init));
            exit(1);
    }
	//inicializamos el semaforo
	sem_init(&semaforo, 0, 1);

	//inicializacion del array de structs y creacion de hilos.
	//pasamos como parametro la posicion del struct segun el hilo.
	for(int i=0; i<num_threads; ++i) {
		RangoCombinaciones[i].index = i;
		RangoCombinaciones[i].combinacion_inicial = CombinacionesAux;
		RangoCombinaciones[i].combinacion_final = CombinacionesAux + CombinacionesPorThread;
		RangoCombinaciones[i].Coste = Coste;
		RangoCombinaciones[i].CosteMejorCombinacion = CosteMejorCombinacion;
		RangoCombinaciones[i].MejorCombinacion = MejorCombinacion;
		//RangoCombinaciones[i].OptimoParcial = OptimoParcial;
		if (pthread_create(&(threads[i]), NULL, (void *(*) (void *)) CombinacionOptimaThread, &(RangoCombinaciones[i]))!=0) {
			perror("Error creación hilos"); 
			exit(1);
		}
		CombinacionesAux += CombinacionesPorThread;
	}
	
	pthread_barrier_wait(&barrier);
	for(int i=0; i<num_threads; ++i) {
		pthread_cancel(threads[i]);
	}
	pthread_barrier_destroy(&barrier);
	
	/*
	//Finalizacion de los hilos.
	for(int i=0; i<num_threads; ++i) {
		pthread_join(threads[i], NULL);
	}
	*/
	
	//Calculo de la combinacion optima entre las optimas resultantes de cada hilo
	for(int i=0; i<num_threads; ++i) {
		Coste = EvaluarCombinacionListaArboles(RangoCombinaciones[i].MejorCombinacion, &Estadisticas[RangoCombinaciones->index]);
		if ( Coste < CosteMejorCombinacion )
		{
			CosteMejorCombinacion = Coste;
			MejorCombinacion = RangoCombinaciones[i].MejorCombinacion;
		}
	}

	ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcialGlobal);
	printf("\rOptimo %ld-> Coste %ld, %d Arboles talados:", MejorCombinacion, CosteMejorCombinacion, OptimoParcialGlobal.NumArboles);
	MostrarArboles(OptimoParcialGlobal);
	printf("\n");

	if (CosteMejorCombinacion == Optimo->Coste) {
		return false;
	}

	// Asignar combinacin encontrada.
	ConvertirCombinacionToArbolesTalados(MejorCombinacion, Optimo);
	Optimo->Coste = CosteMejorCombinacion;
	// Calcular estadisticas óptimo.
	NumArboles = ConvertirCombinacionToArboles(MejorCombinacion, &CombinacionArboles);
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	Optimo->LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);
	MaderaArbolesTalados = CalcularMaderaArbolesTalados(*Optimo);
	Optimo->MaderaSobrante = MaderaArbolesTalados - Optimo->LongitudCerca;
	Optimo->CosteArbolesCortados = CosteMejorCombinacion;
	Optimo->CosteArbolesRestantes = CalcularCosteCombinacion(CombinacionArboles);

	//Imprimir estadisticas parciales y globales finales
	for(int i=0; i<num_threads; ++i) {
		printf("\n+++++++++++++++++++++++ Estadisticas parciales Thread: %d +++++++++++++++++++++++++++++\n", i);
		MostrarEstadisticas(&Estadisticas[i]);
	}
	printf("\n++++++++++++++++++++++++++++++++ Estadisticas globales ++++++++++++++++++++++++++++++++\n");
	MostrarEstadisticas(&EstadisticasGlobales);
	
	//Liberacion de memoria de los hilos.
	free(threads);
	//Liberacion de memoria del array de structs
	free(RangoCombinaciones);
	//Liberacion de memoria de las estadisticas
	free(Estadisticas);

	return true;
}

//Calculo de la combinacion mas optima de cada hilo segun el rango de combinaciones de este
void CombinacionOptimaThread(struct Rango *RangoCombinaciones) {
	int P=0;
	InicializarEstadisticas(&Estadisticas[RangoCombinaciones->index]);
	for (long int Combinacion=RangoCombinaciones->combinacion_inicial; Combinacion<RangoCombinaciones->combinacion_final; ++Combinacion) {
		RangoCombinaciones->Coste = EvaluarCombinacionListaArboles(Combinacion, &Estadisticas[RangoCombinaciones->index]);
		++Estadisticas[RangoCombinaciones->index].EstCombinaciones;
		//Lock mutex
		pthread_mutex_lock(&mutex);
		++EstadisticasGlobales.EstCombinaciones;
		//Unlock mutex
		pthread_mutex_unlock(&mutex);
		if ( RangoCombinaciones->Coste < RangoCombinaciones->CosteMejorCombinacion ) {
			RangoCombinaciones->CosteMejorCombinacion = RangoCombinaciones->Coste;
			RangoCombinaciones->MejorCombinacion = Combinacion;
		}

		if ((Combinacion%S)==0) {
			//Lock mutex
			pthread_mutex_lock(&mutex);
			ConvertirCombinacionToArbolesTalados(RangoCombinaciones->MejorCombinacion, &OptimoParcialGlobal);
			printf("\r[%ld] OptimoParcial %ld-> Coste %ld, %d Arboles talados:", Combinacion, RangoCombinaciones->MejorCombinacion, RangoCombinaciones->CosteMejorCombinacion, OptimoParcialGlobal.NumArboles);
			MostrarArboles(OptimoParcialGlobal);
			//Unlock mutex
			pthread_mutex_unlock(&mutex);
		}
		++P;
		if (P==EstadisticasParciales) {
			//Imprimir estadisticas parciales.
			//Lock mutex
			pthread_mutex_lock(&mutex);
			printf("\n+++++++++++++++++++++++ Estadisticas parciales Thread: %d +++++++++++++++++++++++++++++\n", RangoCombinaciones->index);
			MostrarEstadisticas(&Estadisticas[RangoCombinaciones->index]);
			printf("\n++++++++++++++++++++++++++++++++ Estadisticas globales ++++++++++++++++++++++++++++++++\n");
			MostrarEstadisticas(&EstadisticasGlobales);
			//Unlock mutex
			pthread_mutex_unlock(&mutex);
			P=0;
		}
	}
	pthread_barrier_wait(&barrier);
}

//Funcion para tratar la excepcion CTRL+C
void ExceptionHandler() {
	for(int i=0; i<num_threads; ++i) {
		pthread_cancel(threads[i]);
	}
	exit(0);
}

void InicializarEstadisticas(struct Estadistica *Estadisticas) {
	Estadisticas->EstCombinaciones=1;
	Estadisticas->EstCombValidas=0; 
	Estadisticas->EstCombInvalidas=0; 
	Estadisticas->EstCostePromedio=0;
	Estadisticas->EstMejorCosteCombinacion=DMaximoCoste;
	Estadisticas->EstPeorCosteCombinacion=0;
	Estadisticas->EstMejorArboles=ArbolesEntrada.NumArboles;
	Estadisticas->EstPeorArboles=0;
	Estadisticas->EstMejorArbolesCombinacion=0;
	Estadisticas->EstPeorArbolesCombinacion=0;
	Estadisticas->EstMejorCoste=DMaximoCoste;
	Estadisticas->EstPeorCoste=0;
}

void MostrarEstadisticas(struct Estadistica *Estadisticas) {
	printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("++ Eval Comb: %d \tValidas: %d \tInvalidas: %d\tCoste Validas: %.3f\n", Estadisticas->EstCombinaciones, Estadisticas->EstCombValidas, Estadisticas->EstCombInvalidas, (float)Estadisticas->EstCostePromedio/(float)Estadisticas->EstCombValidas);
	printf("++ Mejor Comb (coste): %d Coste: %.3f  \tPeor Comb (coste): %d Coste: %.3f\n",Estadisticas->EstMejorCosteCombinacion, Estadisticas->EstMejorCoste, Estadisticas->EstPeorCosteCombinacion, Estadisticas->EstPeorCoste);
	printf("++ Mejor Comb (árboles): %d Arboles: %d  \tPeor Comb (árboles): %d Arboles %d\n",Estadisticas->EstMejorArbolesCombinacion, Estadisticas->EstMejorArboles, Estadisticas->EstPeorArbolesCombinacion, Estadisticas->EstPeorArboles);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}

int EvaluarCombinacionListaArboles(int Combinacion, struct Estadistica *Estadisticas)
{
	TVectorCoordenadas CoordArboles, CercaArboles;
	TListaArboles CombinacionArboles, CombinacionArbolesTalados;
	int NumArboles, NumArbolesTalados, PuntosCerca, CosteCombinacion;
	float LongitudCerca, MaderaArbolesTalados;

	// Convertimos la combinacion al vector de arboles no talados.
	NumArboles = ConvertirCombinacionToArboles(Combinacion, &CombinacionArboles);

	// Obtener el vector de coordenadas de arboles no talados.
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);

	// Calcular la cerca
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	/* Evaluar si obtenemos suficientes arboles para construir la cerca */
	LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);

	// Evaluar la madera obtenida mediante los arboles talados.
	// Convertimos la combinacion al vector de arboles no talados.
	NumArbolesTalados = ConvertirCombinacionToArbolesTalados(Combinacion, &CombinacionArbolesTalados);
    if (DDebug) printf(" %d arboles cortados: ",NumArbolesTalados);
    if (DDebug) MostrarArboles(CombinacionArbolesTalados);
    MaderaArbolesTalados = CalcularMaderaArbolesTalados(CombinacionArbolesTalados);
    if (DDebug) printf("  Madera:%4.2f  \tCerca:%4.2f ",MaderaArbolesTalados, LongitudCerca);

    if (LongitudCerca > MaderaArbolesTalados || MaderaArbolesTalados==0 || NumArbolesTalados==0 || NumArboles==0)
    {
        // Los arboles cortados no tienen suficiente madera para construir la cerca.
		++Estadisticas->EstCombInvalidas;
		//Lock
		pthread_mutex_lock(&mutex);
		++EstadisticasGlobales.EstCombInvalidas;
		//Unlock
		pthread_mutex_unlock(&mutex);
        if (DDebug) printf("\tCoste:%d",DMaximoCoste);
            return DMaximoCoste;
    }
    // Evaluar el coste de los arboles talados.
    CosteCombinacion = CalcularCosteCombinacion(CombinacionArbolesTalados);

	// Estadisticas parciales
	++Estadisticas->EstCombValidas;
	Estadisticas->EstCostePromedio+=CosteCombinacion;
	
	if (CosteCombinacion<Estadisticas->EstMejorCoste)
	{
		Estadisticas->EstMejorCoste = CosteCombinacion;
		Estadisticas->EstMejorCosteCombinacion = Combinacion;
	}
	else if (CosteCombinacion>Estadisticas->EstPeorCoste)
	{
		Estadisticas->EstPeorCoste = CosteCombinacion;
		Estadisticas->EstPeorCosteCombinacion = Combinacion;
	}
	if (NumArboles<Estadisticas->EstMejorArboles)
	{
		Estadisticas->EstMejorArboles = NumArboles;
		Estadisticas->EstMejorArbolesCombinacion = Combinacion;
	}
	else if (NumArboles>Estadisticas->EstPeorArboles)
	{
		Estadisticas->EstPeorArboles = NumArboles;
		Estadisticas->EstPeorArbolesCombinacion = Combinacion;
	}
	//Semaforo wait
	sem_wait(&semaforo);
	//Estadisticas globales
	++EstadisticasGlobales.EstCombValidas;
	EstadisticasGlobales.EstCostePromedio+=CosteCombinacion;

	if (CosteCombinacion<EstadisticasGlobales.EstMejorCoste)
	{
		EstadisticasGlobales.EstMejorCoste = CosteCombinacion;
		EstadisticasGlobales.EstMejorCosteCombinacion = Combinacion;
	}
	else if (CosteCombinacion>EstadisticasGlobales.EstPeorCoste)
	{
		EstadisticasGlobales.EstPeorCoste = CosteCombinacion;
		EstadisticasGlobales.EstPeorCosteCombinacion = Combinacion;
	}
	if (NumArboles<EstadisticasGlobales.EstMejorArboles)
	{
		Estadisticas->EstMejorArboles = NumArboles;
		Estadisticas->EstMejorArbolesCombinacion = Combinacion;
	}
	
	else if (NumArboles>EstadisticasGlobales.EstPeorArboles)
	{
		EstadisticasGlobales.EstPeorArboles = NumArboles;
		EstadisticasGlobales.EstPeorArbolesCombinacion = Combinacion;
	}
	//Semaforo wait
	sem_post(&semaforo);

    if (DDebug) printf("\tCoste:%d",CosteCombinacion);
    return CosteCombinacion;
}

int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles)
{
	int arbol=0;

	CombinacionArboles->NumArboles=0;
	CombinacionArboles->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles)
	{
		if ((Combinacion%2)==0)
		{
			CombinacionArboles->Arboles[CombinacionArboles->NumArboles]=arbol;
			CombinacionArboles->NumArboles++;
			CombinacionArboles->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArboles->NumArboles;
}

int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados)
{
	int arbol=0;

	CombinacionArbolesTalados->NumArboles=0;
	CombinacionArbolesTalados->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles)
	{
		if ((Combinacion%2)==1)
		{
			CombinacionArbolesTalados->Arboles[CombinacionArbolesTalados->NumArboles]=arbol;
			CombinacionArbolesTalados->NumArboles++;
			CombinacionArbolesTalados->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArbolesTalados->NumArboles;
}

void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas)
{
	int c, arbol;

	for (c=0;c<CombinacionArboles.NumArboles;c++)
	{
    arbol=CombinacionArboles.Arboles[c];
		Coordenadas[c].x = ArbolesEntrada.Arboles[arbol].Coord.x;
		Coordenadas[c].y = ArbolesEntrada.Arboles[arbol].Coord.y;
	}
}

float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca)
{
	int x;
	float coste;

	for (x=0;x<(SizeCerca-1);x++)
	{
		coste+= CalcularDistancia(CoordenadasCerca[x].x, CoordenadasCerca[x].y, CoordenadasCerca[x+1].x, CoordenadasCerca[x+1].y);
	}

	return coste;
}

float CalcularDistancia(int x1, int y1, int x2, int y2)
{
	return(sqrt(pow((double)abs(x2-x1),2.0)+pow((double)abs(y2-y1),2.0)));
}

int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles)
{
	int a;
	int LongitudTotal=0;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
	{
		LongitudTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Longitud;
	}

	return(LongitudTotal);
}

int CalcularCosteCombinacion(TListaArboles CombinacionArboles)
{
	int a;
	int CosteTotal=0;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
	{
		CosteTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Valor;
	}

	return(CosteTotal);
}

void MostrarArboles(TListaArboles CombinacionArboles)
{
	int a;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
		printf("%d ",ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].IdArbol);

  for (;a<ArbolesEntrada.NumArboles;a++)
    printf("  ");
}
