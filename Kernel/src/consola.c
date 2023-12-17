#include "consola.h"

#define ANSI_COLOR_CYAN "\x1b[36m"
#define RESET_COLOR "\x1b[0m"

extern t_log *kernelLogger;
extern kernel_config *kernelConf;
extern sem_t semReady;
extern pcb *procesoEjecutandoActual;
extern bool finProcesoEjecutando;
extern bool condicionPF;
extern sem_t planificacionPausada;
extern sem_t grado_multiprogramacion;
extern sem_t semNew;
extern sem_t semReady;
extern sem_t semExecute;
extern sem_t fCreate;
extern pthread_mutex_t lista_recursos;
extern sem_t execute_libre;
extern sem_t io;
extern pthread_mutex_t mutex_tabla_archivos;
extern pthread_mutex_t mutexBlocked;
extern pthread_mutex_t mutexNew;
extern pthread_mutex_t mutexReady;
extern pthread_mutex_t mutexExec;
extern pthread_mutex_t mutexExit;
extern pthread_mutex_t mutexLoggerKernel;

extern pcb* pcbPrioridad;

// lista de archivos abiertos

extern pthread_mutex_t mutex_quantum;
extern pthread_mutex_t mutex_pid_global;
extern pthread_mutex_t mutexNodo;

pthread_mutex_t resources;
extern pthread_mutex_t mutexLoggerKernel;
extern bool desalojoexecute;

void operarConsola()
{
	//pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "iniciando conosla");
	//pthread_mutex_unlock(&mutexLoggerKernel);
	int planificacionIniciada = 1;
	pthread_mutex_init(&resources, NULL);
	pcb* pcbauxReady;
	int valorActual;
	while (1)
	{
		printf("ingrese un comando: \n");
		char *leido = readline(">");

		char **split = string_split(leido, " ");
		// int len = string_array_size(split);

		if (string_equals_ignore_case(split[0], "INICIAR_PROCESO"))
		{
			char *pathProceso = (split[1]);
			int sizeProceso = atoi(split[2]);
			int prioridad = atoi(split[3]);
			nuevoProceso(prioridad, sizeProceso, pathProceso);
			//pthread_mutex_lock(&mutexLoggerKernel);
			log_info(kernelLogger, "proceso Creado y instruccion finalizada");
			//pthread_mutex_unlock(&mutexLoggerKernel);
			free(pathProceso);
		}
		else if (string_equals_ignore_case(split[0], "FINALIZAR_PROCESO"))
		{
			int pidProceso = atoi(split[1]);

			pcb *_pcb;
			int pidEx;
			pcb* pcbExec;
			for(int j=0; j<list_size(ready);j++){
					pcbauxReady =list_get(ready,j);
    				sem_getvalue(&semReady, &valorActual);
					if(pcbauxReady->pid ==pidProceso && valorActual != 0){
						sem_wait(&semReady);
					}
				}
			if(list_is_empty(exec)){
				pidEx = -1;
			}else{
				pcbExec = list_get(exec, 0);
				pidEx = pcbExec->pid;
			}
			
			if(pidProceso == pidEx){
                // poner pcb actual en cola de ready
				finProcesoEjecutando = true;
				desalojoexecute = true;
				pcb *next_pcb;
				if(!list_is_empty(pcbExec->resources_taken) ){
					_pcb = list_get(exec, 0);
					list_remove(exec, 0);

					pthread_mutex_lock(&resources);

					for(int i; i < list_size(pcbExec->resources_taken);i++ ){
					pcb *pcb_waiting = signal_deadlock(_pcb, blocked);
					pcb_waiting = pop_blocked(pcb_waiting->pid, blocked);
					cambio_de_estado(pcb_waiting, "READY");
					list_add(ready, pcb_waiting);
					pcbPrioridad = pcb_waiting;
					logger_ready(kernelConf->algoritmo_planificacion);
					sem_post(&semReady);
					}
					
					
								

					list_add(exit_estado, _pcb);
					cambio_de_estado(_pcb, "Exit");
					eliminar_memoria_pcb(_pcb);
					sem_post(&execute_libre);
					pthread_mutex_unlock(&resources);
					
				}else{
					// poner pcb actual en cola de ready
                pthread_mutex_lock(&mutexExit);
                list_add(exit_estado, pcbExec);
				pthread_mutex_unlock(&mutexExit);
				cambio_de_estado(pcbExec, "Exit");
				list_remove(exec, 0);
                eliminar_memoria_pcb(pcbExec);
				desalojoexecute = true;
				//sem_post(&semReady);

				}

				//sem_post(&semReady);
			}
			else{
				_pcb = find_pcb_by_id(pidProceso);
				
				if (_pcb == NULL)
				{
					//pthread_mutex_lock(&mutexLoggerKernel);
					log_warning(kernelLogger, "[%s >> %s] PCB %d not found", __FILE__, __func__, pidProceso);
					//pthread_mutex_unlock(&mutexLoggerKernel);
				}
				else if (is_pcb_on_list(_pcb->pid, deadlocks) == 1) {
					pthread_mutex_lock(&resources);
					pcb *next_pcb = signal_deadlock(_pcb, deadlocks);
					list_add(exit_estado, _pcb);
					cambio_de_estado(_pcb, "Exit");
					_pcb = search_pcb_on_list(deadlocks, _pcb->pid); // solo necesito eliminarlo de deadlocks
					eliminar_memoria_pcb(_pcb);
					pthread_mutex_unlock(&resources);
					release_deadlock(next_pcb);
					detect_deadlock();
					//sem_post(&semReady);
				}
				else{
					list_add(exit_estado, _pcb);
					cambio_de_estado(_pcb, "Exit");
					finProcesoEjecutando = false;
					eliminar_memoria_pcb(_pcb);
					//sem_post(&semReady);
				}
			}
			
		}
		else if (string_equals_ignore_case(split[0], "DETENER_PLANIFICACION"))
		{
			log_info(kernelLogger, "DETENIENDO PLANIFICACION");
			if (planificacionIniciada == 1)
			{
				planificacionIniciada = 0;
				sem_wait(&planificacionPausada);
			}
			else
			{
				//pthread_mutex_lock(&mutexLoggerKernel);
				log_error(kernelLogger, "ERROR: Planificacion ya detenida");
				//pthread_mutex_unlock(&mutexLoggerKernel);
			}
			// todo
		}
		else if (string_equals_ignore_case(split[0], "INICIAR_PLANIFICACION"))
		{
			if (planificacionIniciada == 0)
			{
				planificacionIniciada = 1;
				//pthread_mutex_lock(&mutexLoggerKernel);
				log_info(kernelLogger, "iniciando planificacion");
				//pthread_mutex_unlock(&mutexLoggerKernel);
				sem_post(&planificacionPausada);
			}
			else
			{
				//pthread_mutex_lock(&mutexLoggerKernel);
				log_error(kernelLogger, "ERROR: Planificacion ya iniciada");
				//pthread_mutex_unlock(&mutexLoggerKernel);
			}
			// todo
		}
		else if (string_equals_ignore_case(split[0], "MULTIPROGRAMACION"))
		{
			int nivelMultiprogramacion = atoi(split[1]);
    		sem_getvalue(&grado_multiprogramacion, &valorActual);
			if(kernelConf->grado_max_multiprogramacion > nivelMultiprogramacion){
				log_info(kernelLogger, "Multiprogramacion %d solicitada es menor a la actual %d", nivelMultiprogramacion, kernelConf->grado_max_multiprogramacion);
				int valueFor1 = valorActual - nivelMultiprogramacion;
				for(int i = 0; i < valueFor1; i++){
					sem_wait(&grado_multiprogramacion);
				}
			}
			else if(kernelConf->grado_max_multiprogramacion == nivelMultiprogramacion){
				log_info(kernelLogger, "Multiprogramacion %d solicitada ya es la actual", nivelMultiprogramacion);
			}
			else{
				log_info(kernelLogger, "Multiprogramacion %d solicitada es mayor a la actual %d", nivelMultiprogramacion, kernelConf->grado_max_multiprogramacion);
				int valueFor2 = nivelMultiprogramacion - valorActual;
				for(int i = 0; i < valueFor2; i++){
					sem_post(&grado_multiprogramacion);
				}
			}

			if(nivelMultiprogramacion == 1){
				condicionPF=true;
			}
			else{
				condicionPF=false;
			}

			

			kernelConf->grado_max_multiprogramacion = nivelMultiprogramacion;
		}
		else if (string_equals_ignore_case(split[0], "PROCESO_ESTADO"))
		{
			listarPcb(new, "NEW");
			listarPcb(ready, "READY");
			listarPcb(exec, "EXECUTE");
			listarPcb(exit_estado, "EXIT");
			listarPcb(blocked, "BLOCKED");
		}
		else if (string_equals_ignore_case(split[0], "LIMPIAR_PCBS"))
		{
			pthread_mutex_lock(&mutexExit);
			while(!list_is_empty(exit_estado)){
			pcb* pcbFinal = pop_estado(exit_estado);
			free(pcbFinal);
			}
			list_destroy(exit_estado);
			list_destroy(new);
			list_destroy(ready);
			list_destroy(exec);
			list_destroy(blocked);
			list_destroy(deadlocks);
		}
		free(split);
		free(leido);
	}
}

char *leerArchivo(char *unPath)
{

	char procesos[100];

	strcpy(procesos, "../Memoria/Directorio/");

	strcat(procesos, unPath);

	FILE *file;
	file = fopen(procesos, "r");
	if (file == NULL)
	{
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long int size = ftell(file);
	rewind(file);

	char *content;
	content = calloc(size + 1, 1);

	fread(content, 1, size, file);

	fclose(file);

	return content;
}

pcb* signal_deadlock(pcb *pcb_, t_list* lista)
{
	char* resource = get_deadlock_resource(pcb_);
	int resource_pos = buscar_posicion_recurso(resource);
	if (resource_pos != -1) {
		int max_resources = elementos_max_signal(resource_pos);
		if (max_resources != -1) {
			kernelConf->instancias_recursos[resource_pos] = +1;
			log_info(kernelLogger, "%sPID %d deja el recurso %s con %d%s", ANSI_COLOR_CYAN, pcb_->pid, resource, kernelConf->instancias_recursos[resource_pos], RESET_COLOR);
		}
	}

	log_info(kernelLogger, "%sPOP EN LA COLA N° %d PARA PID %d. RECURSO %s%s", ANSI_COLOR_CYAN, resource_pos, pcb_->pid, resource, RESET_COLOR);
	pcb *pcb_waiting = (pcb*)get_waiting_in_queue(resource_pos, pcb_, lista);
	free(resource);
	return pcb_waiting;
}

void release_deadlock(pcb *pcb_waiting) {
	if (pcb_waiting != NULL) {
		log_info(kernelLogger, "[%s >> %s - Found PCB %d waiting]", __FILE__, __func__, pcb_waiting->pid);
		pcb_waiting = pop_blocked(pcb_waiting->pid, blocked);
		pthread_mutex_lock(&resources);
		pcb_waiting = search_pcb_on_list(deadlocks, pcb_waiting->pid); // solo necesito eliminarlo de deadlocks
		pthread_mutex_unlock(&resources);
		cambio_de_estado(pcb_waiting, "READY");
		list_add(ready, pcb_waiting);
		logger_ready(kernelConf->algoritmo_planificacion);
		sem_post(&semReady);
	}
}
