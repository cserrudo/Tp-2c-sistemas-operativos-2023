#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include "configKernel.h"
#include "estados.h"
#include <pthread.h>
#include <semaphore.h>
#include "algoritmos.h"
#include <buffer/send.h>
#include "instrucciones.h"
#include <commons/collections/queue.h>
#include <deadlock.h>

bool desalojoexecute;

sem_t planificacionPausada;
sem_t grado_multiprogramacion;
sem_t semNew;
sem_t semReady;
sem_t semExecute;
sem_t fCreate;
pthread_mutex_t lista_recursos;
sem_t execute_libre;
sem_t io;
pthread_mutex_t mutex_tabla_archivos;
pthread_mutex_t mutexBlocked;
pthread_mutex_t mutexNew;
pthread_mutex_t mutexReady;
pthread_mutex_t mutexExec;
pthread_mutex_t mutexExit;
pthread_mutex_t mutexLoggerKernel;

pthread_mutex_t mutexPrioridades;

// lista de archivos abiertos

pthread_mutex_t mutex_quantum;
pthread_mutex_t mutex_pid_global;
pthread_mutex_t mutexNodo;

 t_list *esperaWrite;
 t_list *esperaRead;

t_list *new;
t_list *ready;
t_list *exec;
t_list *exit_estado;
t_list *blocked;
t_list *deadlocks;

typedef struct
{
	char *archivo;
	int pid;
	int posicion_puntero;
	char* modoApretura;
} tabla_de_archivos;

typedef struct{
	char* archivo;
	int puntero_posicion;

}puntero_archivo;

void *nuevoProceso(int prioridad, int size, char *path);
void agregarArchivoCola(t_list *lista, pcb *pcbaux);
bool archivoAbiertoLista(t_list *lista, pcb* pcbBuscado);
bool archivoNombreAbiertoLista(t_list *lista, pcb* pcbBuscado);
bool archivoModoAbiertoLista(t_list *lista, pcb* pcbBuscado, char* modo);
bool archivoEnSistema(char* archivo);
void eliminarPcbUsando(t_list *lista, pcb *pcbUsuario);
tabla_de_archivos * findNextTabla(char* modoApreturaAceptado, char* archivo);
bool enUsoModo(t_list *lista,char* archivo, char* modo);
void iniciar_planificacion();
void destruirPlani();
void largo_plazo();
void corto_plazo();
void execute();
int next_pid();
bool lectoresUsando(char* archivo);
void elimniarArchivo(t_list *lista, char *archivoObjetivo);
void actualizarFSeek(pcb* pcbActual);
char* obtenerModoApretura(pcb* Pcbarchivo);
void agregarTablaArchivos(t_list* listaObj, pcb* NuevoMiembro, char* modoApretura);
int obtenerPosicionPuntero(pcb* Pcbarchivo);
void* f_close(pcb* pcbActual);
void agregarTablaArchivos(t_list* listaObj, pcb* NuevoMiembro, char* modoApretura);
void inicio_array_recursos();
pcb* desalojarPrioridad();
void desbloquearIndependientePrioridad(int cantidadDeReady);

#endif
