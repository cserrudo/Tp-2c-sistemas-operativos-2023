#include "estados.h"

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

extern int grado_actual;
extern t_log* kernelLogger;
extern pthread_mutex_t mutexBlocked;
extern pthread_mutex_t mutexExit;
extern pthread_mutex_t mutexReady;
extern pthread_mutex_t mutexLoggerKernel;
extern sem_t grado_multiprogramacion;

extern t_list *new;
extern t_list *ready;
extern t_list *exec;
extern t_list *exit_estado;
extern t_list *blocked;
extern t_list *deadlocks;


pcb* pop_estado(t_list* lista){
    pcb* elem = list_get(lista, 0);
    list_remove(lista, 0);
    return elem;
}

void* push_estado(t_list* lista, pcb* pcb){
    sem_wait(&planificacionPausada);
    sem_post(&planificacionPausada);
    list_add(lista, pcb);
    return NULL;
}

void cambio_de_estado(pcb *PCB, char* nuevo){
	pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", PCB->pid, PCB->status, nuevo);
	pthread_mutex_unlock(&mutexLoggerKernel);
    PCB->status = nuevo;
}

void logger_ready(char* algoritmos) {
	pthread_mutex_lock(&mutexReady);
	char *str = string_new();
    for (int i = 0; i < list_size(ready); i++) {
    	pcb *_pcb = list_get(ready, i);
    	char *pid_str = string_itoa(_pcb->pid);
        string_append(&str, pid_str);
        free(pid_str);
    	if (i < list_size(ready)) {
    		string_append(&str, " ");
    	}
    }
	pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "%sCOLA READY %s: [%s]%s", ANSI_COLOR_GREEN, algoritmos, str, ANSI_COLOR_RESET);
	pthread_mutex_unlock(&mutexLoggerKernel);
	pthread_mutex_unlock(&mutexReady);
	free(str);
}

void* exit_pcb(pcb* pcb, t_list* exit_estado,char* origen){
    grado_actual -= 1;
    cambio_de_estado(pcb, "exit");
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "enviando a memoria que finalize el proceso");
	pthread_mutex_unlock(&mutexLoggerKernel);
	f_close(pcb);
    eliminar_memoria_pcb(pcb); 
	
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "proceso finalizado completamente");
	pthread_mutex_unlock(&mutexLoggerKernel);
	sem_post(&grado_multiprogramacion);
    return NULL;
}

pcb* pop_blocked(int pid, t_list* blocked) {
    pthread_mutex_lock(&mutexBlocked);
    pcb* pcbUnblocked = search_pcb_on_list(blocked, pid); // todo corregir
    if (pcbUnblocked == NULL) {
		pthread_mutex_lock(&mutexLoggerKernel);
        log_error(kernelLogger, "No hay PCBs con el PID buscado en blocked!");
		pthread_mutex_unlock(&mutexLoggerKernel);
		pthread_mutex_unlock(&mutexBlocked);
        return NULL; 
    }

	pthread_mutex_lock(&mutexLoggerKernel);
     log_info(kernelLogger, "Se liberó el PCB con PID %d", pcbUnblocked->pid);
	 pthread_mutex_unlock(&mutexLoggerKernel);
	 pthread_mutex_unlock(&mutexBlocked);
     return pcbUnblocked;
	 
 }

pcb* search_pcb_on_list(t_list *lista, int pid) { //lo devuelve y lo saca
	pcb *pcbAux ;
	for (int i = 0; i < list_size(lista); i++) {
		pcbAux = (pcb *)list_get(lista, i);
		if (pcbAux->pid == pid) {
			// Usamos el status como el nombre de la lista
			pthread_mutex_lock(&mutexLoggerKernel);
			log_info(kernelLogger, "[%s >> %s] PCB %d found", __FILE__, __func__, pcbAux->pid);
			pthread_mutex_unlock(&mutexLoggerKernel);
			list_remove(lista, i);
			return pcbAux;
		}
	}
	return NULL;
}

pcb* find_pcb_by_id(int pid) {
	pcb *_pcb;
	_pcb = search_pcb_on_list(new, pid);
	if (_pcb == NULL) {
		_pcb = search_pcb_on_list(ready, pid);
		if (_pcb == NULL) {
			_pcb = search_pcb_on_list(exec, pid);
			if (_pcb == NULL) {
				_pcb = search_pcb_on_list(blocked, pid);
			}
		}
	}
	return _pcb;
}

int is_pcb_on_list(int pid, t_list *list) {
	pcb *pcbAux ;
	for (int i = 0; i < list_size(list); i++) {
		pcbAux = (pcb *)list_get(list, i);
		if (pcbAux->pid == pid) {
			return 1;
		}
	}
	return 0;
}


void listarPcb(t_list *estado, char *nombre) {
	if (estado != NULL)
	{
		for (int i = 0; i < list_size(estado); i++)
		{
			pcb *pcbEstado = list_get(estado, i);
			pthread_mutex_lock(&mutexLoggerKernel);
			log_info(kernelLogger, "Estado: %s - PCB: %d", nombre,pcbEstado->pid);
			pthread_mutex_unlock(&mutexLoggerKernel);
		}
	}
}



