#ifndef CONFIGMEMORIA_H_
#define CONFIGMEMORIA_H_

#include <commons/config.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <netdb.h>
#include <commons/log.h>
#include <buffer/buffer.h>
#include <server/server.h>
#include <structs/structs.h>
#include <commons/config.h>
#include <utils/flags.h>
#include "peticiones.h"
#include <utils/mix.h>
#include <handshakes/hs.h>
#include "gestion_de_memoria.h"

typedef struct
{
    char *ip_filesystem;
    char *puerto_filesystem;
    char *puerto_escucha;
    int tam_memoria;
    int tam_pagina;
    char *path_instrucciones;
    int retardo_respuesta;
    char *algoritmo_reemplazo;
    int socket_fs;
    int socket_cpu;
    int socket_kernel;

} memoria_config;

extern t_log *memoriaLogger;
memoria_config *memoria_config_crear(char *path);
void memoria_config_iniciar(void *moduleConfig, t_config *config);
void hacerConexiones(memoria_config *memoriaConfig);

#endif
