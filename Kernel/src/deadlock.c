
#include <deadlock.h>


#define ANSI_COLOR_CYAN "\x1b[36m"
#define RESET_COLOR "\x1b[0m"

extern t_log *kernelLogger;
extern kernel_config *kernelConf;
extern pthread_mutex_t mutexLoggerKernel;


void detect_deadlock()
{
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "%s[%s >> %s] Starting deadlock detection%s", ANSI_COLOR_CYAN, __FILE__, __func__, RESET_COLOR);\
	pthread_mutex_unlock(&mutexLoggerKernel);

	t_list *posible_deadlocks = list_create();
	//t_list *posible_deadlocks = list_duplicate(blocked);
	list_add_all(posible_deadlocks, blocked);
	list_add_all(posible_deadlocks, ready);
	list_add_all(posible_deadlocks, exec);
	t_list *aux_list = list_create();

	while (!list_is_empty(posible_deadlocks))
	{
		pcb *to_check = (pcb *)list_remove(posible_deadlocks, 0);
		circular_wait(to_check, posible_deadlocks, aux_list);
	}
	pthread_mutex_lock(&mutexLoggerKernel);
	log_info(kernelLogger, "%s[%s >> %s] Finished deadlock detection%s", ANSI_COLOR_CYAN, __FILE__, __func__, RESET_COLOR);
	pthread_mutex_unlock(&mutexLoggerKernel);

}

void trace_deadlock(pcb *_pcb)
{
	int counter = 0;
	for (int i = 0; i < list_size(deadlocks); i++) {
		pcb *aux = (pcb*)list_get(deadlocks, i);
		if (aux->pid == _pcb->pid) {
			return ;
		}
	}
	if (counter == 0) {
		list_add(deadlocks, _pcb);
		pthread_mutex_lock(&mutexLoggerKernel);
		log_info(kernelLogger, "[%s >> %s] Deadlock added. PCB %d", __FILE__, __func__, _pcb->pid);
		pthread_mutex_unlock(&mutexLoggerKernel);
	}
}

int can_release_deadlock(char *resource_requested, pcb *to_evaluate)
{
	for (int i = 0; i < list_size(to_evaluate->resources_taken); i++) {
		char *resource_taken = list_get(to_evaluate->resources_taken, i);
		int position = buscar_posicion_recurso(resource_taken);
		int resource_instances = kernelConf->instancias_recursos[position];
		if (strcmp(resource_requested, resource_taken) == 0 && resource_instances > 0) {
			return 1;
		}
	}
	return 0;
}

int check_owned_resources(char *resource_requested, pcb *to_evaluate)
{
	for (int i = 0; i < list_size(to_evaluate->resources_taken); i++) {
		char *resource_taken = list_get(to_evaluate->resources_taken, i);
		if (strcmp(resource_requested, resource_taken) == 0) {
			return 1;
		}
	}
	return 0;
}

bool check_deadlock(pcb *aux, pcb *current_pcb, int resource_instances)
{
	if (check_owned_resources(current_pcb->resource_requested, aux) == 1 && resource_instances <= 0 && strcmp(current_pcb->status, "BLOCKED") == 0)
	{
		log_warning(kernelLogger, "Deadlock detectado: PID %d - Recursos en posesión: %s - Recurso requerido: %s", current_pcb->pid, resources_taken_to_str(aux), current_pcb->resource_requested);
		trace_deadlock(current_pcb);
		return true;
	}
	return false;
}

void circular_wait(pcb *to_check, t_list *posible_deadlocks, t_list *aux_list)
{
    list_add(aux_list, (pcb *)to_check);
    int *index = malloc(sizeof(int));

    if (find_processes_holding_same_resource(to_check, aux_list, index))
    {
    	log_info(kernelLogger, "[%s >> %s] Found on aux_list", __FILE__, __func__);
        free(index);
        return ;
    }
    if (find_processes_holding_same_resource(to_check, posible_deadlocks, index))
    {
    	log_info(kernelLogger, "[%s >> %s] Found on posible_deadlocks (modified)", __FILE__, __func__);
    	to_check = list_remove(posible_deadlocks, *index);
        free(index);
        circular_wait(to_check, posible_deadlocks, aux_list);
    }
    else
    {
        free(index);
        return ;
    }
}

bool find_processes_holding_same_resource(pcb *to_check, t_list *list, int *index)
{
	log_info(kernelLogger, "[%s >> %s] Search for processes holding the resource: %s", __FILE__, __func__, to_check->resource_requested);

	int position = buscar_posicion_recurso(to_check->resource_requested);
	if (position == -1)
	{
		log_warning(kernelLogger, "[%s >> %s] Resource %s does not exist. PID: %d", __FILE__, __func__, to_check->resource_requested, to_check->pid);
	}
	int resource_instances = kernelConf->instancias_recursos[position];

	pcb *aux;
	int i = 0;
	bool found = false;

	if (!list_is_empty(list))
	{
		while (i < list_size(list) && !found)
		{
			aux = list_get(list, i);
			if (check_deadlock(aux, to_check, resource_instances)) {
				*index = i;
				found = true;
			}
			i++;
		}
		return found;
	}
	return false;
}

char *get_deadlock_resource(pcb *pcb)
{
	if (list_is_empty(pcb->resources_taken)) {
		char* resource = malloc(strlen(pcb->recurso) + 1);
		strcpy(resource, pcb->recurso);
		return resource;
	}
	char *first_resource = list_get(pcb->resources_taken, 0);
	char *resource = malloc(strlen(first_resource) + 1);
	strcpy(resource, first_resource);
	return resource;
}

char *resources_taken_to_str(pcb *pcb) {
	char *str = string_new();
	for (int i = 0; i < list_size(pcb->resources_taken); i++) {
		char *resource = list_get(pcb->resources_taken, i);
		string_append(&str, resource);
		string_append(&str, " ");
	}
	return str;
}


int are_waiting_for_resource(char *resource, t_list *list) {
	for (int i = 0; i < list_size(list); i++) {
		pcb *aux = (pcb*)list_get(list, i);
		if (strcmp(resource, aux->resource_requested) == 0) {
			return 1;
		}
	}
	return 0;
}
