#include "configCpu.h"

extern cpu_config *cpuConf;
extern t_log *cpuLogger;
extern pthread_mutex_t *mutex_interrupt;

pthread_t th_dispatch;
pthread_t th_interrupt;

cpu_config *cpu_config_crear(char *path)
{
	cpu_config *config_aux = malloc(sizeof(*config_aux));
	config_init(config_aux, path, cpuLogger, cpu_config_iniciar);
	return config_aux;
}

void cpu_config_iniciar(void *moduleConfig, t_config *config)
{
	cpu_config *cpuConfig = (cpu_config *)moduleConfig;
	//cpuConfig->ip_memoria= strdup(config_get_string_value(config, "IP_MEMORIA"));
	cpuConfig->puerto_memoria = strdup(config_get_string_value(config, "PUERTO_MEMORIA"));
	cpuConfig->puerto_escucha_dispatch = strdup(config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH"));
	cpuConfig->puerto_escucha_interrupt = strdup(config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT"));
	cpuConfig->tam_pagina = 0;
	cpuConfig->socket_kernel_dispatch = -1;
	cpuConfig->socket_kernel_interrumpt = -1;
	cpuConfig->socket_memoria = -1;
}


void iniciarCpu(){
	memoria_cpu_data*  data_cpu = recibir_memoria_cpu(cpuConf->socket_memoria);
	cpuConf->tam_pagina = data_cpu->programCounter;
	free(data_cpu);
	log_info(cpuLogger, "se recibio el tamanio de pagina %d", cpuConf->tam_pagina);
}

void hacerConexiones(cpu_config *cpuConf)
{
	// pthread_mutex_init(&mutex_interrupt, NULL); esto para que es?

	// levanto el serverCpu
	int cpuServerDispatch = iniciar_servidor(cpuConf->puerto_escucha_dispatch);
	int cpuServerInterrupt = iniciar_servidor(cpuConf->puerto_escucha_interrupt);

	// socket memoria

	cpuConf->socket_memoria = conectar_a_servidor(cpuConf->ip_memoria, cpuConf->puerto_memoria);

	if (cpuConf->socket_memoria == -1)
	{
		log_error(cpuLogger, "error socket en coneccion de cpu dipatch con kernel");
		// cpu_destruir(cpuConf, cpuLogger);
		exit(-1);
	}

	enviarHS(cpuConf->socket_memoria, hs_memoria, cpuLogger, "Memoria", "Cpu");

	

	// socket cpu dispatch

	cpuConf->socket_kernel_dispatch = aceptar_conexion_server(cpuServerDispatch);

	recibirHS(cpuConf->socket_kernel_dispatch, hs_cpu_dispatch, cpuLogger, "Kernel", "Cpu Dispatch");

	// socket cpu Interrupt

	cpuConf->socket_kernel_interrumpt = aceptar_conexion_server(cpuServerInterrupt);

	recibirHS(cpuConf->socket_kernel_interrumpt, hs_cpu_interrupt, cpuLogger, "Kernel", "Cpu Interrupt");

	iniciarCpu();

	pthread_create(&th_dispatch, NULL, (void *)atender_dispatch, NULL);

	pthread_create(&th_interrupt, NULL, (void *)atender_interrupt, NULL);

	pthread_join(th_dispatch, NULL);
	pthread_join(th_interrupt, NULL);
}

