#ifndef ALGORITMOS_H_
#define ALGORITMOS_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include "configKernel.h"
#include "estados.h"
#include <pthread.h>
#include <semaphore.h>


pcb* prioridades(t_list* estado);
pcb* procesoConMayorPrioridad(t_list* ready,int  prioridadProcesoActual);
void rr_quantum_start(int pid);
void terminarQuantum();
bool procesoConMayorPrioridadChequeo(t_list *ready, int prioridadProcesoActual);
#endif
