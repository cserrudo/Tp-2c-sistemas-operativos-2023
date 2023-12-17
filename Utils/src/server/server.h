#ifndef SERVER_H_
#define SERVER_H_
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include <semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <commons/temporal.h>
#include <errno.h>
#include "../buffer/buffer.h"


typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

int iniciar_servidor(char* puerto);

int iniciar_servidor(char* puerto);
t_list* recibir_paquete(int socket_cliente);
void __stream_send(int toSocket, void* streamToSend, int bufferSize);
void* __stream_create(uint8_t header, t_buffer* buffer, int bytes);
void stream_send_buffer(int toSocket, uint8_t header, t_buffer* buffer) ;
void stream_send_empty_buffer(int toSocket, uint8_t header);
void stream_recv_buffer(int fromSocket, t_buffer* destBuffer);
uint8_t stream_recv_header(int fromSocket);
int aceptar_conexion_server(int socket);
void* recibir_buffer(int* size, int socket_cliente);
void stream_recv_empty_buffer(int fromSocket);
#endif
