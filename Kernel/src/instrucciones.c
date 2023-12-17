#include "instrucciones.h"

extern kernel_config *kernelConf;
extern t_log *kernelLogger;

extern sem_t io;
extern sem_t semReady;
extern sem_t fCreate;
extern bool cpu_ejecutando;
extern pthread_mutex_t mutexLoggerKernel;
bool finProcesoEjecutando;
extern bool desalojoexecute;
extern bool condicionPF;
extern  int prioridadProcesoActual;
extern bool condicionPrioridad;
extern sem_t interruptFunco;
extern t_list *blocked;
extern t_list* ready;

#define ANSI_COLOR_CYAN "\x1b[36m"
#define RESET_COLOR "\x1b[0m"
#define CANTIDAD_RECURSOS 10
t_queue *arrayRecursos[CANTIDAD_RECURSOS];


// recursos

int count_elements(char **array)
{
	int count = 0;
	while (array[count] != NULL)
	{
		count++;
	}
	return count;
}

int buscar_posicion_recurso(char *recurso)
{
	char **arrayRecursos = kernelConf->recurso;	   // huardo el array en una variable para su uso
	int sizeArray = count_elements(arrayRecursos); // busco tamanio del array recursos
	char *newline = strchr(recurso, '\n');
	if (newline != NULL)
	{
		*newline = '\0';
	}
	for (int i = 0; i < sizeArray; i++)
	{
		if (strcmp(arrayRecursos[i], recurso) == 0)
		{ // esto no me esta
			pthread_mutex_lock(&mutexLoggerKernel);
			log_info(kernelLogger, "se encontro la posicion del recurso %s en %d", recurso, i);
			pthread_mutex_unlock(&mutexLoggerKernel);
			return i;
		}
	}
	return -1;
}

void recurso_inexistente(char *recursoPcb, pcb *pcb)
{
	pthread_mutex_lock(&mutexLoggerKernel);
	log_error(kernelLogger, "recurso %s no existe", recursoPcb);
	pthread_mutex_unlock(&mutexLoggerKernel);
}

int hay_elementos_wait(int posicion)
{
//	if (kernelConf->instancias_recursos[posicion] < 0) {
//		return -2;
//	}
//	kernelConf->instancias_recursos[posicion] -= 1;
//	if (kernelConf->instancias_recursos[posicion] < 0) {
//		return -1;
//	}
	kernelConf->instancias_recursos[posicion] -= 1;
	if (kernelConf->instancias_recursos[posicion] < 0) { return -1 ;}
	log_info(kernelLogger, "pase, hay elementos wait");
	return 1;
}

int elementos_max_signal(int posicion)
{
	if (kernelConf->instancias_recursos[posicion] >= kernelConf->recursosIniciales[posicion])
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

char *wait(pcb *pcb)
{
	char *recursoDelPcb = pcb->recurso;
	int resultadoWait = buscar_posicion_recurso(recursoDelPcb);
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "encontre posicion recurso %d", resultadoWait);
	pthread_mutex_unlock(&mutexLoggerKernel);
	// chequeo si el elemento existe
	if (resultadoWait == -1)
	{
		log_error(kernelLogger, "ERROR: recurso solicitado no se encuentra en el sistema :(");
		recurso_inexistente(recursoDelPcb, pcb);
		return "no existe recurso";
	}
	else
	{
		log_info(kernelLogger, "se encontro el recurso solicitado por la instruccion Wait, chequeando si hay suficientes recursos para la operacion ...");
	}

	log_info(kernelLogger, "cantidad de instancias del recurso %d", kernelConf->instancias_recursos[resultadoWait]);
	kernelConf->instancias_recursos[resultadoWait] -= 1;
//	int resultadoOperacion = hay_elementos_wait(resultadoWait);

	if (kernelConf->instancias_recursos[resultadoWait] < 0)
	{
		log_warning(kernelLogger, "Instancias del recurso %s son negativas: %d", recursoDelPcb, kernelConf->instancias_recursos[resultadoWait]);
		pcb->programCounter -= 1;
		log_info(kernelLogger, "%sPUSHEANDO PID %d A COLA N° %d - RECURSO %s%s", ANSI_COLOR_CYAN, pcb->pid, resultadoWait, recursoDelPcb, RESET_COLOR);
		queue_push(arrayRecursos[resultadoWait], pcb);
		return "falta recursos";
	}
	else
	{
		list_add(pcb->resources_taken, recursoDelPcb);
		log_info(kernelLogger, "PID: %d - Wait: %s - Instancias: %d", pcb->pid, pcb->recurso, kernelConf->instancias_recursos[resultadoWait]);
		return "hay recursos suficiente";
	}
}

char *i_signals(pcb *pcb1)
{
	char *recursoDelPcb = pcb1->recurso;
	int resultadoSignal = buscar_posicion_recurso(recursoDelPcb);
	if (resultadoSignal == -1)
	{
		log_error(kernelLogger, "ERROR: recurso solicitado no se encuentra en el sistema :( ");
		recurso_inexistente(recursoDelPcb, pcb1);
		return "no existe recurso";
	}
	else
	{
		log_info(kernelLogger, "se encontro el recurso solicitado por la instruccion Signal! :) ");
	}
	int resultadoMax = elementos_max_signal(resultadoSignal);
	if (resultadoMax == -1)
	{
		log_error(kernelLogger, "ERROR: cantidad maxima de recursos alcanzada");
		log_error(kernelLogger, ">> RECURSOS MAXIMOS PARA PCB %d - Recurso %s - Cantidad %d", pcb1->pid, pcb1->recurso, kernelConf->instancias_recursos[resultadoSignal]);
		pcb1->programCounter -=1 ;
		return "max recursos";
	}
	kernelConf->instancias_recursos[resultadoSignal] = +1;
	log_info(kernelLogger, "PID: %d - Signal: %s - Instancias: %d", pcb1->pid, recursoDelPcb, kernelConf->instancias_recursos[resultadoSignal]);

	for (int i = 0; i < list_size(pcb1->resources_taken); i++) {
		char *resource = list_get(pcb1->resources_taken, i);
		if (strcmp(resource, recursoDelPcb) == 0) {
			list_remove(pcb1->resources_taken, i);
			log_warning(kernelLogger, "REMOVIENDO EL RECURSO %s DEL PCB %d", resource, pcb1->pid);
		}
	}

	log_info(kernelLogger, "%sPOP EN LA COLA N° %d PARA PID %d. RECURSO %s%s", ANSI_COLOR_CYAN, resultadoSignal, pcb1->pid, recursoDelPcb, RESET_COLOR);
	int resource_pos = buscar_posicion_recurso(recursoDelPcb);
	pcb *pcb_waiting = (pcb*)get_waiting_in_queue(resource_pos, pcb1, blocked);
	if (pcb_waiting != NULL && pcb_waiting->pid != pcb1->pid && !finProcesoEjecutando) {
		log_info(kernelLogger, "[%s >> %s - Found PCB %d waiting for resource: %s]", __FILE__, __func__, pcb_waiting->pid, recursoDelPcb);
		pcb_waiting = pop_blocked(pcb_waiting->pid, blocked);
		cambio_de_estado(pcb_waiting, "READY");
		push_estado(ready, pcb_waiting);
		logger_ready(kernelConf->algoritmo_planificacion);
		sem_post(&semReady);
	}

	return "recursos liberado";
}

// sleep

void sleepHilo(pcb *pcb1)
{
	int sleepValue = pcb1->sleep;
	usleep(sleepValue);
	pop_blocked(pcb1->pid, blocked);
	push_estado(ready, pcb1);
	cambio_de_estado(pcb1, "READY");
	logger_ready(kernelConf->algoritmo_planificacion);
	sem_post(&semReady);
}

void instruccion_en_sleep(pcb *pcb)
{
	push_estado(blocked, pcb);
	cambio_de_estado(pcb, "BLOCKED");
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "PID: %d - Bloqueado por: SLEEP", pcb->pid);
	log_info(kernelLogger, "PID %d - Ejecuta IO: %d", pcb->pid, pcb->sleep);
	pthread_mutex_unlock(&mutexLoggerKernel);
	pthread_t ejecutarIO;
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "entrado a sleep");
	pthread_mutex_unlock(&mutexLoggerKernel);
	pthread_create(&ejecutarIO, NULL, (void *)sleepHilo, (void *)pcb);
	pthread_detach(ejecutarIO);

	return;
}

// memoria

void crear_memoria_pcb(int id, char *nombre, int size)
{
	kernel_memoria_data *info = malloc(sizeof(*info));
	info->pid = id;
	info->nombre = nombre;
	info->size = size;
	info->pagina = -1;
	enviar_kernel_memoria(info, kernelConf->socket_memoria, INICIAR_PROCESO_KERNEL);
}
void eliminar_memoria_pcb(pcb *pcbAux)
{
	kernel_memoria_data *info = malloc(sizeof(*info));
	pcb *pcbNuevo = malloc(sizeof(*pcbNuevo));
	info->pid = pcbAux->pid;
	char* vacio = "";
	info->nombre = vacio;
	info->pagina = -1;
	info->size = -1;
	if (cpu_ejecutando && finProcesoEjecutando)
	{
		pthread_mutex_lock(&mutexLoggerKernel);
		log_info(kernelLogger, "cpu ejecutando, parandolo para desalojar");
		pthread_mutex_unlock(&mutexLoggerKernel);
		enviar_cpu_kernel(pcbAux, kernelConf->socket_cpu_interrupt, desalojar_pcb);
		condicionPrioridad = true;
		
		
		pthread_mutex_lock(&mutexLoggerKernel);
		log_info(kernelLogger, "se desalojo con exito");
		pthread_mutex_unlock(&mutexLoggerKernel);
		finProcesoEjecutando = true;
	}
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "enviando a memoria que finalize el proceso");
	pthread_mutex_unlock(&mutexLoggerKernel);
	enviar_kernel_memoria(info, kernelConf->socket_memoria, FINALIZAR_PROCESO);
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "se envio el exit");
	pthread_mutex_unlock(&mutexLoggerKernel);
	//free(pcbAux);
	free(pcbNuevo);
	//free(info);
}

void pageFaultKerenel(pcb *pcbAux)
{
	pthread_mutex_lock(&mutexExec);
	pop_estado(exec);
	pthread_mutex_unlock(&mutexExec);
	list_add(blocked, pcbAux);
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "PID: %d - Bloqueado por: PAGEFAULT", pcbAux->pid);
	pthread_mutex_unlock(&mutexLoggerKernel);
	// log_info(kernelLogger, "Page Fault PID: %d - Pagina: %d", pcbAux->pid, pcbAux->pagina);
	pthread_t pageFaultHilo;
	pthread_create(&pageFaultHilo, NULL, (void *)pageFaultFuncion, pcbAux);
	pthread_detach(pageFaultHilo);
}

void pageFaultFuncion(pcb *pcbAux)
{
	kernel_memoria_data *data = malloc(sizeof(kernel_memoria_data));
	data->flag = SOLUCIONAR_PAGE_FAULT;
	data->pid = pcbAux->pid;
	data->pagina = pcbAux->direccion;
	char* vacio = "";
	data->nombre = vacio;
	data->size=-1;
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "enviando datos a memoria para resolver el PF");
	pthread_mutex_unlock(&mutexLoggerKernel);
	enviar_kernel_memoria(data, kernelConf->socket_memoria, SOLUCIONAR_PAGE_FAULT);
	/*kernel_memoria_data* data2 = recibir_kernel_memoria(kernelConf->socket_memoria);
	pcbAux->direccion = data2->pagina;
	push_estado(ready, pcbAux);
	cambio_de_estado(pcbAux, "READY");
	logger_ready(kernelConf->algoritmo_planificacion);
	sem_post(&semReady);*/
	//free(data);
}

// filesystem

void f_open_instruccion(char *nombreArchvio)
{
	fs_kernel_data *data = malloc(sizeof(*data));
	data->nombreArchivo =strdup(nombreArchvio);
	data->direcFisica=-1;
	data->info=-1;
	data->pid=-1;
	data->puntero=-1;
	data->tamanioNuevo=-1;
	enviar_fs_kernel(data, kernelConf->socket_filesystem, i_open);
	sem_wait(&fCreate);
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "se abrio el archivo con exito");
	pthread_mutex_unlock(&mutexLoggerKernel);
}



void desbloquearPcbFS(){
	//me pasa el pid y desbloqueo el pcb segun esto.
	while(1){
	fs_kernel_data* data2 = recibir_fs_kernel(kernelConf->socket_filesystem);
	switch(data2->flag){
		case archivo_inexsitente:
		enviar_fs_kernel(data2,kernelConf->socket_filesystem, i_create );
		break;

		case creacion_ok:
		sem_post(&fCreate);
		break;

		case ejecuta_ok:

		//desbloquear el siguiente pcb bloqueado por fs
		// desbloqueo y mando a ready
		pcb* pcbDesbloqueado = pop_blocked(data2->pid, blocked);
		free(data2);
		push_estado(ready, pcbDesbloqueado);
        cambio_de_estado(pcbDesbloqueado, "READY");
        logger_ready(kernelConf->algoritmo_planificacion);
		if(strcmp(kernelConf->algoritmo_planificacion, "PRIORIDADES") == 0 && procesoConMayorPrioridadChequeo(ready, prioridadProcesoActual)){
			//sem_post(&semReady);
			desalojarPrioridad(-1);
		}
		else{
			sem_post(&semReady);
		}
		
        
		break;
	}
	}
	
}

void desbloquearPcbMemoria(){
	//me pasa el pid y desbloqueo el pcb segun esto.
	int valorActual;
	while(1){
	kernel_memoria_data* data2 = recibir_kernel_memoria(kernelConf->socket_memoria);
	switch(data2->flag){
		case SOLUCIONAR_PAGE_FAULT:

		//desbloquear el siguiente pcb bloqueado por fs
		// desbloqueo y mando a ready
		pcb* pcbDesbloqueado = pop_blocked(data2->pid, blocked);
		pcbDesbloqueado->direccion = data2->pagina;
		pcbDesbloqueado->programCounter--;
		push_estado(ready, pcbDesbloqueado);
        cambio_de_estado(pcbDesbloqueado, "READY");
		if(strcmp(kernelConf->algoritmo_planificacion, "PRIORIDADES") == 0 && procesoConMayorPrioridadChequeo(ready, prioridadProcesoActual)){
			//sem_post(&semReady);
			desalojarPrioridad(-1);
		}
		else{
        sem_post(&semReady);
		
		
		
		if(list_size(ready) == 1 && list_size(exec)==0 && condicionPF){
			sem_post(&execute_libre);
		}
		}
		break;
	}
	}
	
}

pcb* get_waiting_in_queue(int resource_pos, pcb *pcb_, t_list *list)
{
	int waiting = are_waiting_for_resource(pcb_->recurso, list);
	if (waiting == 0) return NULL;

	t_list *pcbs = list_create();
	pcb *pcb;
	t_queue *queue = arrayRecursos[resource_pos];

	while (queue_size(queue) > 0) {
		pcb = queue_pop(queue);
		if (pcb == NULL) break;
		if (pcb->pid != pcb_->pid && strcmp(pcb_->recurso, pcb->resource_requested) == 0) break;
		list_add(pcbs, pcb);
	}

	for (int i = 0; i < list_size(pcbs); i++) {
		queue_push(queue, list_get(pcbs, i));
	}

	return pcb;
}
