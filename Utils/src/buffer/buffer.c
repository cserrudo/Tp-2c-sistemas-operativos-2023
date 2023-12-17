#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>

t_buffer* buffer_create(void) {
    t_buffer* self = malloc(sizeof(*self));
    self->size = 0;
    self->stream = NULL;
    return self;
}

t_buffer* buffer_create_copy(t_buffer* bufferToCopy) {
    t_buffer* self = malloc(sizeof(*self));
    self->size = bufferToCopy->size;
    self->stream = malloc(self->size);
    memcpy(self->stream, bufferToCopy->stream, self->size);
    return self;
}

void buffer_destroy(t_buffer* self) {
    free(self->stream);
    free(self);
}

int buffer_pack(t_buffer* self, void* streamToAdd, int size, int offset) {
	void *tmp = realloc(self->stream, self->size + size);
	self->stream = tmp;
    memcpy(self->stream + self->size, streamToAdd, size);
    self->size += size;
    return size + offset;
}

int buffer_unpack(t_buffer* self, void* dest, int size, int offset) {
    if (self->stream == NULL || self->size == 0) {
        puts("\e[0;31mbuffer_unpack: Error en el desempaquetado del buffer\e[0m");
        exit(-1);
    }
    memcpy(dest, self->stream + offset, size);
    return offset + size;
}

int buffer_pack_string(t_buffer* self, char* stringToAdd, int offset) {
    uint32_t length = strlen(stringToAdd) + 1;
    offset = buffer_pack(self, &length, sizeof(length), offset);
    offset = buffer_pack(self, stringToAdd, length * sizeof(char), offset);
    return offset + sizeof(length) + sizeof(stringToAdd);
}

char* buffer_unpack_string(t_buffer* self, int *offset) {
	uint32_t length;
    *offset = buffer_unpack(self, &length, sizeof(length), *offset);
    char *str = malloc(length * sizeof(char));
    *offset = buffer_unpack(self, str, length * sizeof(char), *offset);
    return str;
}


int buffer_pack_list(t_list* lista, t_buffer* buffer, int offset){
    if(lista != NULL){
    int listSize = list_size(lista);
    offset = buffer_pack(buffer, &listSize, sizeof(int), offset);
    for(int i = 0; i< list_size(lista); i++){
		void* dato =  list_get(lista, i);
        offset = buffer_pack(buffer, &dato, sizeof(dato), offset);
	}
    }
    else{
        offset = buffer_pack(buffer, -1, sizeof(int), offset);
    }
    return offset;
}


t_list* buffer_unpack_list(t_buffer* buffer, int offset){
    t_list* lista_nueva = list_create();
    int sizeLista;
    offset = buffer_unpack(buffer, &sizeLista, sizeof(sizeLista), offset);
    if(sizeLista != -1){
    void* elem = NULL;
    for(int i = 0; i< sizeLista; i++){
        
		offset = buffer_unpack(buffer, &elem, sizeof(elem), offset);
        list_add(lista_nueva, elem);
	}
    return lista_nueva;
    }
    else{
        return NULL;
    }
}


int buffer_pack_list_char(t_list *lista, t_buffer *buffer, int offset) {
	if (lista != NULL) {
		int listSize = list_size(lista);
		offset = buffer_pack(buffer, &listSize, sizeof(int), offset);
		for (int i = 0; i < list_size(lista); i++) {
			void *dato = list_get(lista, i);
			offset = buffer_pack_string(buffer, dato, offset);
		}
	} else {
		offset = buffer_pack(buffer, -1, sizeof(int), offset);
	}
	return offset;
}

t_list* buffer_unpack_list_char(t_buffer *buffer, int *offset) {
	t_list *lista_nueva = list_create();
	int sizeLista;
	*offset = buffer_unpack(buffer, &sizeLista, sizeof(sizeLista), *offset);
	if (sizeLista != -1) {
		void *elem = NULL;
		for (int i = 0; i < sizeLista; i++) {
			elem = buffer_unpack_string(buffer, offset);
			list_add(lista_nueva, elem);
		}
		return lista_nueva;
	} else {
		return NULL;
	}
}

