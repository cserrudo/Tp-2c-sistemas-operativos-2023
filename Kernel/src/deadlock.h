#ifndef DEADLOCK_H_
#define DEADLOCK_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include <configKernel.h>
#include <estados.h>
#include <pthread.h>
#include <semaphore.h>
#include <algoritmos.h>
#include <buffer/send.h>
#include <instrucciones.h>
#include <commons/collections/queue.h>

void detect_deadlock();
bool check_deadlock(pcb *aux, pcb *current_pcb, int resource_instances);
void circular_wait(pcb *to_check, t_list *posible_deadlocks, t_list *aux_list);
bool find_processes_holding_same_resource(pcb *to_check, t_list *list, int *index);
char *resources_taken_to_str(pcb *pcb);
char *get_deadlock_resource(pcb *pcb);
int can_release_deadlock(char *resource_requested, pcb *to_evaluate);
int check_owned_resources(char *resource_requested, pcb *to_evaluate);
int are_waiting_for_resource(char *resource, t_list *list);

#endif
