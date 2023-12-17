#ifndef SEND_H_
#define SEND_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <semaphore.h>
#include "../structs/structs.h"
#include "../server/server.h"
#include "../utils/flags.h"
#include <buffer/buffer.h>
#include <commons/string.h>


void enviar_cpu_kernel(pcb* pcb, int socket, kernel_cpu_dato flag);
pcb* recibir_cpu_kernel(int socket);
void enviar_interrupt_cpu_kernel(int socket, kernel_cpu_dato flag);
kernel_cpu_dato recibir_interrupt_cpu_kernel(int socket);
void enviar_kernel_memoria(kernel_memoria_data* data, int socket, client_flag flag);
kernel_memoria_data* recibir_kernel_memoria(int socket);
memoria_cpu_data* recibir_memoria_cpu(int socket);
void enviar_memoria_cpu(memoria_cpu_data* data, int socket, client_flag flag);
void enviar_fs_memoria(fs_memoria_data* data, int socket, memoria_fs_instruccion flag);
fs_memoria_data* recibir_fs_memoria(int socket);
void enviar_fs_kernel(fs_kernel_data* data, int socket, kernel_fs_instruccion flag);
fs_kernel_data* recibir_fs_kernel(int socket);
void liberarPcb(pcb *proceso);

#endif
