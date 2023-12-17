#ifndef ESTADO_H_
#define ESTADO_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include "configKernel.h"
#include "instrucciones.h"
#include <structs/structs.h>
#include "planificador.h"

//para block, van a estar en cada archivo abierto.

pcb* pop_estado(t_list* lista);
void* push_estado(t_list* lista, pcb* pcb);
void cambio_de_estado(pcb *PCB, char* nuevo);
pcb* pop_blocked(int pid, t_list* blocked);
void estado_exit_normal(pcb* pcbAux, t_list* listaExit);
void* exit_pcb(pcb* pcb, t_list* exit_estado,char* origen);
pcb* find_pcb_by_id(int pid);
void listarPcb(t_list* estado, char* nombre);
pcb* search_pcb_on_list(t_list* lista, int pid);
void logger_ready(char* algoritmos);
int is_pcb_on_list(int pid, t_list *list);

#endif
