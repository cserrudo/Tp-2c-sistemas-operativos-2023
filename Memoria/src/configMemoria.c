#include "configMemoria.h"

memoria_config* memoriaConfig;

memoria_config *memoria_config_crear(char *path)
{
	memoria_config *config_aux = malloc(sizeof(*config_aux));
	config_init(config_aux, path, memoriaLogger, memoria_config_iniciar);
	return config_aux;
}

void memoria_config_iniciar(void *moduleConfig, t_config *config)
{
	memoria_config *memoriaConfig = (memoria_config *)moduleConfig;
	memoriaConfig->puerto_filesystem = strdup(config_get_string_value(config, "PUERTO_FILESYSTEM"));
	memoriaConfig->puerto_escucha = strdup(config_get_string_value(config, "PUERTO_ESCUCHA"));
	memoriaConfig->ip_filesystem = strdup(config_get_string_value(config, "IP_FILESYSTEM"));
	memoriaConfig->tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	memoriaConfig->tam_pagina = config_get_int_value(config, "TAM_PAGINA");
	memoriaConfig->path_instrucciones = strdup(config_get_string_value(config, "PATH_INSTRUCCIONES"));
	memoriaConfig->retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
	memoriaConfig->algoritmo_reemplazo = strdup(config_get_string_value(config, "ALGORITMO_REEMPLAZO"));
	memoriaConfig->socket_cpu = -1;
	memoriaConfig->socket_fs = -1;
	memoriaConfig->socket_kernel = -1;
}

void enviarTamanioPagina(){
	
	memoria_cpu_data* data_cpu = malloc(sizeof(memoria_cpu_data));

	data_cpu->programCounter = memoriaConfig->tam_pagina;
	data_cpu->param1 = "";
	data_cpu->param2 = "";
	data_cpu->param3 = "";
	data_cpu->pid = -1;
	data_cpu->registroValor =0;
	data_cpu->direccion = -1;
	log_info(memoriaLogger, "enviando tam pagina %d", memoriaConfig->tam_pagina);
	enviar_memoria_cpu(data_cpu,memoriaConfig->socket_cpu,INICIAR_PROCESO_CPU);

}

void hacerConexiones(memoria_config *memoriaConfig)
{

	// levanto el server memoria
	int memoriaServer = iniciar_servidor(memoriaConfig->puerto_escucha);
	log_info(memoriaLogger, "iniciando servicdor");

	// socket filesystem
	memoriaConfig->socket_fs = aceptar_conexion_server(memoriaServer);
	log_info(memoriaLogger, "Se acepto conexion con fs");
	recibirHS(memoriaConfig->socket_fs, hs_memoria, memoriaLogger, "Filesystem", "Memoria");
	log_info(memoriaLogger, "Se finalizo el hs con fs");
	// socket cpu
	memoriaConfig->socket_cpu = aceptar_conexion_server(memoriaServer);
	recibirHS(memoriaConfig->socket_cpu, hs_memoria, memoriaLogger, "Cpu", "Memoria");
	log_info(memoriaLogger, "Se finalizo el hs con cpu");


	// socket kernel
	memoriaConfig->socket_kernel = aceptar_conexion_server(memoriaServer);
	recibirHS(memoriaConfig->socket_kernel, hs_memoria, memoriaLogger, "Kernel", "Memoria");
	log_info(memoriaLogger, "Se finalizo el hs con kernel");

	enviarTamanioPagina();
	

	pthread_t hilo_fs;
	pthread_create(&hilo_fs, NULL, (void *)procesar_conexion_fs, &(memoriaConfig->socket_fs));
	pthread_detach(hilo_fs);

	pthread_t hilo_cpu;
	pthread_create(&hilo_cpu, NULL, (void *)procesar_conexion_cpu, &(memoriaConfig->socket_cpu));
	pthread_detach(hilo_cpu);

	pthread_t hilo_kernel;
	pthread_create(&hilo_kernel, NULL, (void *)procesar_conexion_kernel, &(memoriaConfig->socket_kernel));
	pthread_join(hilo_kernel, NULL);
}


