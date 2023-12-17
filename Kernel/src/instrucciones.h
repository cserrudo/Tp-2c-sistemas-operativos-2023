#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include <commons/log.h>
#include <commons/collections/list.h>
#include "configKernel.h"
#include "estados.h"
#include <pthread.h>
#include <semaphore.h>
#include "algoritmos.h"
#include <buffer/send.h>
#include <commons/collections/queue.h>
#include <estados.h>

int buscar_posicion_recurso(char* recurso);
void recurso_inexistente(char*recursoPcb,pcb* pcb);
int hay_elementos_wait(int posicion);
char* wait(pcb* pcb);
char* i_signals(pcb* pcb);
void signal_on_finish(pcb *_pcb);
void crear_memoria_pcb(int id, char* nombre, int size);
void instruccion_en_sleep(pcb* pcb);
void sleepHilo(pcb* pcb1);
void eliminar_memoria_pcb(pcb* pcbAux);
void pageFaultKerenel(pcb* pcbAux);
void f_open_instruccion(char *nombreArchvio);
void pageFaultFuncion(pcb* pcbAux);
int elementos_max_signal(int posicion);
void desbloquearPcbFS();
void desbloquearPcbMemoria();
pcb *get_waiting_in_queue(int resource_pos, pcb *pcb, t_list *list);


#endif
