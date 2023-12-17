#ifndef CICLO_DE_INSTRUCCION_H_
#define CICLO_DE_INSTRUCCION_H_

#include <commons/config.h>
#include<commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include<netdb.h>
#include<commons/log.h>
#include <structs/structs.h>
#include <buffer/send.h>
#include "configCpu.h"


t_instruccion* fetch(pcb *pcb);
void ejecutar_ciclo_de_instruccion(pcb *pcb, int socket_kernel);
t_instruccion *convertir_data_a_instruccion(memoria_cpu_data* data_cpu);
t_instruccion* fetch(pcb *pcb);
uint32_t* obtenerRegistro(registros_cpu *registros, char *nombreRegistro);
void ejecutar_set(pcb* pcb, t_instruccion* instruccion);
void ejecutar_wait(char* recurso, pcb* pcb);
void ejecutar_signal(char* recurso, pcb* pcb);
void decode(t_instruccion *instruccion, pcb* pcb);
bool chequear_si_hay_interrupcion() ;
void ejecutar_ciclo_de_instruccion(pcb *pcb, int socket_kernel);
void ejecutar_fread(char* archivo, uint32_t dir_logica, pcb* pcbAux);
void ejecutar_fwrite(char* archivo, uint32_t dir_logica, pcb* pcbAux);
bool condicionSigoEjecutando(t_identificador identificador);

#endif
