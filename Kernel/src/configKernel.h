#ifndef CONFIGKERNEL_H_
#define CONFIGKERNEL_H_
#include <string.h>
#include <commons/config.h>
#include<commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include<netdb.h>
#include<commons/log.h>
#include <structs/structs.h>
#include <utils/mix.h>
#include <server/cliente.h>
#include <handshakes/hs.h>
#include <planificador.h>


typedef struct {
	char* ip_memoria;
	char* ip_filesystem;
	char* ip_cpu;
	char* puerto_memoria;
	char* puerto_filesystem;
	char* puerto_cpu_dispatch;
	char* puerto_cpu_interrupt;
	char* algoritmo_planificacion;
	int quantum;
	int grado_max_multiprogramacion;
	char** recurso; 
	int* instancias_recursos;
	int* recursosIniciales;
	int socket_cpu_dispatch;
	int socket_cpu_interrupt;
	int socket_memoria;
	int socket_filesystem;
	pthread_t* threadQuantum;
}kernel_config;


void kernel_config_iniciar(void* moduleConfig, t_config* config);
kernel_config* kernel_config_crear(char* path);
void hacerConexiones(kernel_config* kernelConf);


#endif