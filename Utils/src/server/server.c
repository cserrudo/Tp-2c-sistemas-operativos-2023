#include "server.h"
#include "../buffer/buffer.h"


int recibir_operacion(int socket_cliente)
{
	int cod_op;

	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		perror("Ocurrio un error al momento de recibir un mensaje");
		return -1;
	}
}



int iniciar_servidor(char* puerto){
	// Quitar esta línea cuando hayamos terminado de implementar la funcion
	//assert(!"no implementado!");

	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo-> ai_family,
							servinfo->ai_socktype,
							servinfo->ai_protocol);
	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	return socket_servidor;
}


t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

void __stream_send(int toSocket, void* streamToSend, int bufferSize) {
    uint8_t header = 0;
    uint32_t size = 0;

    ssize_t bytesSent = send(toSocket, streamToSend, bufferSize, 0);
    if (bytesSent == -1) {
        printf("\e[0;31m__stream_send: Error en el envío del buffer [%s]\e[0m\n", strerror(errno));
    }
}

void* __stream_create(uint8_t header, t_buffer* buffer, int bytes){
    void* streamToSend = malloc(bytes);
    int offset = 0;
    memcpy(streamToSend + offset, &header, sizeof(header));
    offset += sizeof(header);
    memcpy(streamToSend + offset, &(buffer->size), sizeof(buffer->size));
    offset += sizeof(buffer->size);
    memcpy(streamToSend + offset, buffer->stream, buffer->size);
    return streamToSend;
}

void stream_send_buffer(int toSocket, uint8_t header, t_buffer* buffer) {
	int bytes = buffer->size + sizeof(header) + sizeof(buffer->size);
    void* stream = __stream_create(header, buffer, bytes);
    __stream_send(toSocket, stream, bytes);
    free(stream);
}

void stream_send_empty_buffer(int toSocket, uint8_t header) {
    t_buffer* emptyBuffer = buffer_create();
    stream_send_buffer(toSocket, header, emptyBuffer);
    buffer_destroy(emptyBuffer);
}

void stream_recv_buffer(int fromSocket, t_buffer* destBuffer) {
    ssize_t msgBytes = recv(fromSocket, &(destBuffer->size), sizeof(destBuffer->size), 0);
    if (msgBytes <= 0) {
        if (msgBytes == 0) {
            printf("\e[0;31mstream_recv_buffer: La conexión ha sido cerrada por el otro extremo\e[0m\n");
        } else {
            printf("\e[0;31mstream_recv_buffer: Error en la recepción del buffer [%s]\e[0m\n", strerror(errno));
        }
        exit(-1);
    } else if (destBuffer->size > 0) {
        destBuffer->stream = malloc(destBuffer->size);
        if (destBuffer->stream == NULL) {
            printf("\e[0;31mstream_recv_buffer: Error en la asignación de memoria\e[0m\n");
            exit(-1);
        }
        recv(fromSocket, destBuffer->stream, destBuffer->size, 0);
    }
}

uint8_t stream_recv_header(int fromSocket) {
    uint8_t header;
    ssize_t msgBytes = recv(fromSocket, &header, sizeof(header), 0);
    if (msgBytes == -1) {
        printf("\e[0;31mstream_recv_buffer: Error en la recepción del header [%s]\e[0m\n", strerror(errno));
    }
    return header;
}

int aceptar_conexion_server(int socket){
	struct sockaddr cliente = {0};
	socklen_t aux = sizeof(cliente);
	int cliente_aux = accept(socket, &cliente, &aux);
		if(cliente_aux > -1){
			int* socketCliente = malloc(sizeof(*socketCliente));
			*socketCliente = cliente_aux;
			free(socketCliente);
			return cliente_aux;
		}
		
		return -1;
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}


void stream_recv_empty_buffer(int fromSocket) {
    t_buffer* buffer = buffer_create();
    stream_recv_buffer(fromSocket, buffer);
    buffer_destroy(buffer);
}
