#include "configKernel.h"
#include <commons/config.h>
#include <utils/flags.h>

extern t_log* kernelLogger;
extern pthread_mutex_t mutexLoggerKernel;
extern bool condicionPF;

kernel_config* kernel_config_crear(char* path){
	kernel_config* config_aux = malloc(sizeof(*config_aux));
	 pthread_mutex_lock(&mutexLoggerKernel);
	config_init(config_aux, path, kernelLogger, kernel_config_iniciar); 
	 pthread_mutex_unlock(&mutexLoggerKernel);
	return config_aux;
}

void kernel_config_iniciar(void* moduleConfig, t_config* config){
	kernel_config* kernelConfig = (kernel_config*)moduleConfig; 
	kernelConfig->ip_memoria = strdup(config_get_string_value(config, "IP_MEMORIA"));
	kernelConfig->ip_cpu = strdup(config_get_string_value(config, "IP_CPU"));
	kernelConfig->ip_filesystem = strdup(config_get_string_value(config, "IP_FILESYSTEM"));
	kernelConfig->puerto_cpu_dispatch = strdup(config_get_string_value(config, "PUERTO_CPU_DISPATCH"));
	kernelConfig->puerto_cpu_interrupt = strdup(config_get_string_value(config, "PUERTO_CPU_INTERRUPT"));
	kernelConfig->puerto_filesystem = strdup(config_get_string_value(config, "PUERTO_FILESYSTEM"));
	kernelConfig->puerto_memoria = strdup(config_get_string_value(config, "PUERTO_MEMORIA"));
	kernelConfig->algoritmo_planificacion = strdup(config_get_string_value(config, "ALGORITMO_PLANIFICACION"));
	kernelConfig->quantum = config_get_int_value(config, "QUANTUM");
	kernelConfig->recurso = config_get_array_value(config, "RECURSOS"); 
	kernelConfig->grado_max_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION_INI");
	if(kernelConfig->grado_max_multiprogramacion == 1){
		condicionPF = true;
	}
	else{
		condicionPF = false;
	}
	char** aux = config_get_array_value(config, "INSTANCIAS_RECURSOS");
	int size = 0;
	while(aux[size]!= NULL){
		size++;
	}
	kernelConfig->instancias_recursos = (int*)malloc(size * sizeof(int));
	for(int i= 0; i<size; i++){
		kernelConfig->instancias_recursos[i] = atoi(aux[i]);
	}
	size = 0;
	while(aux[size]!= NULL){
		size++;
	}
	kernelConfig->recursosIniciales = (int*)malloc(size * sizeof(int));
	for(int i= 0; i<size; i++){
		kernelConfig->recursosIniciales[i] = atoi(aux[i]);
	}
	kernelConfig->socket_cpu_dispatch = -1;
	kernelConfig->socket_cpu_interrupt = -1;
	kernelConfig->socket_memoria = -1;
	kernelConfig->socket_filesystem = -1;
}


void hacerConexiones(kernel_config* kernelConf){


	//socket memoria
    kernelConf->socket_memoria = conectar_a_servidor(kernelConf->ip_memoria, kernelConf->puerto_memoria);
    if (kernelConf->socket_memoria == -1) {
		 	pthread_mutex_lock(&mutexLoggerKernel);
			log_error(kernelLogger, "error socket en coneccion de memoria con kernel");
			 pthread_mutex_unlock(&mutexLoggerKernel);
			//kernel_destruir(kernelConf, kernelLogger);
			exit(-1);
	}

    enviarHS(kernelConf->socket_memoria, hs_memoria,kernelLogger, "memoria", "kernel");

	

	//socket cpu dispatch


    kernelConf->socket_cpu_dispatch = conectar_a_servidor(kernelConf->ip_cpu , kernelConf->puerto_cpu_dispatch);
    if (kernelConf->socket_cpu_dispatch == -1) {
		 	pthread_mutex_lock(&mutexLoggerKernel);
			log_error(kernelLogger, "error socket en coneccion de cpu dipatch con kernel");
			 pthread_mutex_unlock(&mutexLoggerKernel);
			//kernel_destruir(kernelConf, kernelLogger);
			exit(-1);
	}

    enviarHS(kernelConf->socket_cpu_dispatch, hs_cpu_dispatch ,kernelLogger, "cpu dispatch", "kernel");


    //socket cpu interrupt
	kernelConf->socket_cpu_interrupt = conectar_a_servidor(kernelConf->ip_cpu , kernelConf->puerto_cpu_interrupt);
    if (kernelConf->socket_cpu_interrupt == -1) {
		 	pthread_mutex_lock(&mutexLoggerKernel);
			log_error(kernelLogger, "error socket en coneccion de cpu interrupt con kernel");
			 pthread_mutex_unlock(&mutexLoggerKernel);
			//kernel_destruir(kernelConf, kernelLogger);
			exit(-1);
	}

    enviarHS(kernelConf->socket_cpu_interrupt, hs_cpu_interrupt,kernelLogger, "cpu interrupt", "kernel");

    //socket filesystem
    
	kernelConf->socket_filesystem = conectar_a_servidor(kernelConf->ip_filesystem , kernelConf->puerto_filesystem);

    if (kernelConf->socket_filesystem == -1) {
		 pthread_mutex_lock(&mutexLoggerKernel);
			log_error(kernelLogger, "error socket en coneccion de filesystem con kernel");
			 pthread_mutex_unlock(&mutexLoggerKernel);
			//kernel_destruir(kernelConf, kernelLogger);
			exit(-1);
	}
 pthread_mutex_lock(&mutexLoggerKernel);
    enviarHS(kernelConf->socket_filesystem, hs_filesystem,kernelLogger, "filesystem", "kernel");
	 pthread_mutex_unlock(&mutexLoggerKernel);

	


}
