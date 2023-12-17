#include "configFS.h"
#include <commons/config.h>
#include <utils/flags.h>

extern t_log* fsLogger;


fs_config* fs_config_crear(char* path){
	fs_config* config_aux = malloc(sizeof(*config_aux));
	config_init(config_aux, path, fsLogger, fs_config_iniciar); 
	return config_aux;
}

void fs_config_iniciar(void* moduleConfig, t_config* config){
	fs_config*  fsConfig = (fs_config*)moduleConfig; 
	fsConfig->ip_memoria = strdup(config_get_string_value(config, "IP_MEMORIA"));
	fsConfig->puerto_memoria= strdup(config_get_string_value(config, "PUERTO_MEMORIA"));
	fsConfig->puerto_escucha = strdup(config_get_string_value(config, "PUERTO_ESCUCHA"));
    fsConfig->path_fat = strdup(config_get_string_value(config, "PATH_FAT"));
    fsConfig->path_bloques = strdup(config_get_string_value(config, "PATH_BLOQUES"));
    fsConfig->path_fcb = strdup(config_get_string_value(config, "PATH_FCB"));
    fsConfig->cant_bloques_total = config_get_int_value(config, "CANT_BLOQUES_TOTAL");
    fsConfig->cant_bloques_swap = config_get_int_value(config, "CANT_BLOQUES_SWAP");
    fsConfig->tam_bloque = config_get_int_value(config, "TAM_BLOQUE");
    fsConfig->retardo_acceso_bloque = config_get_int_value(config, "RETARDO_ACCESO_BLOQUE");
	fsConfig->retardo_acceso_fat = config_get_int_value(config, "RETARDO_ACCESO_FAT");
	fsConfig->socket_kernel = -1;
	fsConfig->socket_memoria = -1;
}

/*
void hacerConexiones(fs_config* fsConfig){

	//levanto el server filesystem
	int fsServer =  iniciar_servidor(fsConfig->puerto_escucha);

	//socket memoria

    fsConfig->socket_memoria = conectar_a_servidor(fsConfig->ip_memoria , fsConfig->puerto_memoria);

    if (fsConfig->socket_memoria == -1) {
			log_error(fsLogger, "error socket en coneccion de cpu dipatch con kernel");
			//kernel_destruir(fsConfig, fsLogger);
			exit(-1);
	}

    enviarHS(fsConfig->socket_memoria, hs_memoria ,fsLogger, "Memoria", "Filesystem");


    //socket kernel

	fsConfig->socket_kernel = aceptar_conexion_server(fsServer);
	recibirHS(fsConfig->socket_kernel, hs_filesystem, fsLogger, "Kernel", "Filesystem");



}*/
void hacerConexiones(fs_config* fsConfig){

	//levanto el server filesystem
	int fsServer =  iniciar_servidor(fsConfig->puerto_escucha);

	//socket memoria
	char* ipMemoria = fsConfig->ip_memoria;
	char* puertoMemoria = fsConfig->puerto_memoria;

    fsConfig->socket_memoria = conectar_a_servidor(ipMemoria, puertoMemoria);
	
	log_info(fsLogger, "Se realizo conexion con servidor");
    if (fsConfig->socket_memoria == -1) {
			log_error(fsLogger, "error socket en coneccion de fs con memoria");
			//kernel_destruir(fsConfig, fsLogger);
			exit(-1);
	}
	log_info(fsLogger, "enviando hs a memoria");
	 
    enviarHS(fsConfig->socket_memoria, hs_memoria ,fsLogger, "Memoria", "Filesystem");
	log_info(fsLogger, "Se envio hs a memoria");

    //socket kernel
	log_info(fsLogger, "CONECTANDO a kernel");
	
	fsConfig->socket_kernel = aceptar_conexion_server(fsServer); 
	log_info(fsLogger, "se conecto a kernel");
	log_info(fsLogger, "enviando hs a kernel");
	
	recibirHS(fsConfig->socket_kernel, hs_filesystem, fsLogger, "Kernel", "Filesystem");
	log_info(fsLogger, "Se envio hs a kernel");


}
