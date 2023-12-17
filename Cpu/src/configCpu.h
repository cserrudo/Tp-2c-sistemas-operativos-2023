#ifndef CONFIGCPU_H_
#define CONFIGCPU_H_

#include <commons/config.h>
#include<commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include<netdb.h>
#include<commons/log.h>
#include "peticiones.h"
#include <utils/mix.h>
#include <server/server.h>
#include <handshakes/hs.h>
#include <server/cliente.h>


typedef struct {
	char* ip_memoria;
	char* puerto_memoria;
	int retardo_instruccion;
	char* puerto_escucha_dispatch;
	char* puerto_escucha_interrupt;
	int tam_max_interrupt;
	int tam_pagina;
	int socket_memoria;
	int socket_kernel_dispatch;
	int socket_kernel_interrumpt;

}cpu_config;



cpu_config* cpu_config_crear(char* path);
void cpu_config_iniciar(void* moduleConfig, t_config* config);
void hacerConexiones(cpu_config* cpuConf);

#endif
