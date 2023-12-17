#ifndef CONSOLA_H_
#define CONSOLA_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include "configKernel.h"
#include "instrucciones.h"
#include "planificador.h"
#include "estados.h"
#include <readline/readline.h>


char* leerArchivo(char* unPath);

void release_deadlock(pcb *pcb_waiting);
void pop_waiting_pcb(pcb *freed_pcb);
void operarConsola();
pcb* signal_deadlock(pcb *pcb_, t_list* lista);

#endif
