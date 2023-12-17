#ifndef CONFIGFS_H_
#define CONFIGFS_H_

#include <commons/config.h>
#include<commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include<netdb.h>
#include<commons/log.h>
#include <utils/mix.h>
#include <handshakes/hs.h>
#include <server/server.h>
#include <server/cliente.h>


typedef struct {
	char* ip_memoria;
	char* puerto_memoria;
    char* puerto_escucha;
    char* path_fat;
    char* path_bloques;
    char* path_fcb;
    int cant_bloques_total;
    int cant_bloques_swap;
    int tam_bloque;
	int retardo_acceso_bloque;
    int retardo_acceso_fat;
	int socket_memoria;
	int socket_kernel;

}fs_config;


fs_config* fs_config_crear(char* path);
void fs_config_iniciar(void* moduleConfig, t_config* config);
void hacerConexiones(fs_config* fsConfig);


#endif