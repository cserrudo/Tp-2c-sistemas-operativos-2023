#include "send.h"


// enviar cpu kernel dispatch

void enviar_cpu_kernel(pcb* pcb, int socket, kernel_cpu_dato flag){
   t_buffer* bufferEnviar =  buffer_create();
    int aux = pcb->pid;
    int offset = 0;
    offset = buffer_pack(bufferEnviar, &aux, sizeof(int), offset);
    aux = pcb->prioridad;
    offset = buffer_pack(bufferEnviar, &aux, sizeof(int), offset);
    aux = pcb->programCounter;
    offset = buffer_pack(bufferEnviar, &aux, sizeof(int), offset);
    uint32_t aux2 =  pcb->registros_cpu->AX;
    offset = buffer_pack(bufferEnviar, &aux2, sizeof(uint32_t), offset);
    aux2 = pcb->registros_cpu->BX;
    offset = buffer_pack(bufferEnviar, &aux2, sizeof(uint32_t), offset);
    aux2 = pcb->registros_cpu->CX;
    offset = buffer_pack(bufferEnviar, &aux2, sizeof(uint32_t), offset);
    aux2 = pcb->registros_cpu->DX;
    offset = buffer_pack(bufferEnviar, &aux2, sizeof(uint32_t), offset);
    offset = buffer_pack_string(bufferEnviar,  pcb->archivo, offset);
    aux2 = pcb->direccion;
    offset = buffer_pack(bufferEnviar,  &aux2, sizeof(uint32_t), offset);
    aux =  pcb->puntero_size;
    offset = buffer_pack(bufferEnviar, &aux, sizeof(int), offset);
    offset = buffer_pack_string(bufferEnviar,  pcb->recurso, offset);
    offset = buffer_pack_string(bufferEnviar, pcb->resource_requested, offset);
    offset = buffer_pack_list_char(pcb->resources_taken, bufferEnviar, offset);
    offset = buffer_pack_string(bufferEnviar, pcb->file_requested, offset);
    offset = buffer_pack_string(bufferEnviar, pcb->status, offset);
    aux = pcb->sleep;
    offset = buffer_pack(bufferEnviar,&aux, sizeof(int), offset);
    stream_send_buffer(socket, flag, bufferEnviar);
    buffer_destroy(bufferEnviar);
}

pcb* recibir_cpu_kernel(int socket){
    t_buffer* bufferEnviar =  buffer_create();
   
    kernel_cpu_dato received_flag = stream_recv_header(socket);
    
    stream_recv_buffer(socket, bufferEnviar);
    
    pcb* pcb1 = malloc(sizeof(pcb));
    int offset = 0;
    pcb1->registros_cpu = malloc(sizeof(registros_cpu));
    int aux;
    offset = buffer_unpack(bufferEnviar, &aux, sizeof(int), offset);
    pcb1->pid = aux;
    offset = buffer_unpack(bufferEnviar, &pcb1->prioridad, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->programCounter, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->registros_cpu->AX, sizeof(uint32_t), offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->registros_cpu->BX, sizeof(uint32_t), offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->registros_cpu->CX, sizeof(uint32_t), offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->registros_cpu->DX, sizeof(uint32_t), offset);
    pcb1->archivo = buffer_unpack_string(bufferEnviar, &offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->direccion, sizeof(uint32_t), offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->puntero_size, sizeof(int), offset);
    pcb1->recurso = buffer_unpack_string(bufferEnviar, &offset);
    pcb1->resource_requested = buffer_unpack_string(bufferEnviar, &offset);
    pcb1->resources_taken = buffer_unpack_list_char(bufferEnviar, &offset);
    pcb1->file_requested = buffer_unpack_string(bufferEnviar, &offset);
    pcb1->status = buffer_unpack_string(bufferEnviar, &offset);
    offset = buffer_unpack(bufferEnviar, &pcb1->sleep, sizeof(pcb1->sleep), offset);
    buffer_destroy(bufferEnviar);
    pcb1->flag = received_flag;
    return pcb1;
}


// interrupt kernel- cpu

void enviar_interrupt_cpu_kernel(int socket, kernel_cpu_dato flag){
    t_buffer* bufferEnviar =  buffer_create();
    stream_send_buffer(socket, flag, bufferEnviar);
    buffer_destroy(bufferEnviar);
}

kernel_cpu_dato recibir_interrupt_cpu_kernel(int socket){
    t_buffer* bufferEnviar =  buffer_create();
    kernel_cpu_dato flag = stream_recv_header(socket);
    stream_recv_buffer(socket, bufferEnviar);
    buffer_destroy(bufferEnviar);
    return flag;
}


// memoria - kernel


void enviar_kernel_memoria(kernel_memoria_data* data, int socket, client_flag flag){
    t_buffer* bufferEnviar =  buffer_create();
    int offset = 0;
    offset = buffer_pack(bufferEnviar, &data->pagina, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &data->pid, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &data->size, sizeof(int), offset);
    offset = buffer_pack_string(bufferEnviar, data->nombre, offset);
    stream_send_buffer(socket, flag, bufferEnviar);
    buffer_destroy(bufferEnviar); //aca muere
    free(data);
}

kernel_memoria_data* recibir_kernel_memoria(int socket){
    t_buffer* bufferEnviar =  buffer_create();
    kernel_memoria_data* data = malloc(sizeof(kernel_memoria_data));
    data->flag = stream_recv_header(socket);
    stream_recv_buffer(socket, bufferEnviar);
    int offset = 0;
    offset = buffer_unpack(bufferEnviar, &data->pagina, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &data->pid, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &data->size, sizeof(int), offset);
    data->nombre = buffer_unpack_string(bufferEnviar, &offset);
    buffer_destroy(bufferEnviar);
    return data;
}

memoria_cpu_data* recibir_memoria_cpu(int socket){
    t_buffer* bufferEnviar =  buffer_create();
    memoria_cpu_data* data = malloc(sizeof(memoria_cpu_data));
    data->flag = stream_recv_header(socket);
    stream_recv_buffer(socket, bufferEnviar);
    int offset = 0;
    offset = buffer_unpack(bufferEnviar, &(data->programCounter), sizeof(int), offset);
    data->param1 = buffer_unpack_string(bufferEnviar, &offset);
    data->param2 = buffer_unpack_string(bufferEnviar, &offset);
    data->param3 = buffer_unpack_string(bufferEnviar, &offset);
    offset = buffer_unpack(bufferEnviar, &(data->pid), sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &(data->direccion), sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &(data->registroValor), sizeof(uint32_t), offset);
    buffer_destroy(bufferEnviar);
    return data;
}
void enviar_memoria_cpu(memoria_cpu_data* data, int socket, client_flag flag){
    t_buffer* bufferEnviar =  buffer_create();
    int offset = 0;
    offset = buffer_pack(bufferEnviar, &data->programCounter, sizeof(int), offset);
    offset = buffer_pack_string(bufferEnviar, data->param1, offset);
    offset = buffer_pack_string(bufferEnviar, data->param2, offset);
    offset = buffer_pack_string(bufferEnviar, data->param3, offset);
    offset = buffer_pack(bufferEnviar, &data->pid, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &data->direccion, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &data->registroValor, sizeof(uint32_t), offset);
    stream_send_buffer(socket, flag, bufferEnviar);
    buffer_destroy(bufferEnviar);
}

void enviar_fs_memoria(fs_memoria_data* data, int socket, memoria_fs_instruccion flag){
    t_buffer* bufferEnviar =  buffer_create();
    int cantEntradas = data->cantEntradas;
    int pid = data->pid;
    int direcFs = data->direcFisica;
    uint32_t info = data->info;
    uint32_t numBloque =data->numero_de_bloque;
    int offset = 0;
    offset = buffer_pack(bufferEnviar, &cantEntradas, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &pid, sizeof(int), offset);
    offset = buffer_pack_list(data->bloques_asignados, bufferEnviar, offset);
    offset = buffer_pack(bufferEnviar, &direcFs, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &info, sizeof(uint32_t), offset);
    offset = buffer_pack(bufferEnviar, &numBloque, sizeof(uint32_t), offset);
    stream_send_buffer(socket, flag, bufferEnviar);
    free(data);
    buffer_destroy(bufferEnviar);
}

fs_memoria_data* recibir_fs_memoria(int socket){
    t_buffer* bufferEnviar =  buffer_create();
    fs_memoria_data* data = malloc(sizeof(fs_memoria_data));
    data->flag = stream_recv_header(socket);
    stream_recv_buffer(socket, bufferEnviar);
    int offset = 0;
    offset = buffer_unpack(bufferEnviar, &data->cantEntradas, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &data->pid, sizeof(int), offset);
    data->bloques_asignados = buffer_unpack_list(bufferEnviar, offset);
    offset = buffer_unpack(bufferEnviar, &data->direcFisica, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &data->info, sizeof(uint32_t), offset);
    offset = buffer_unpack(bufferEnviar, &data->numero_de_bloque, sizeof(uint32_t), offset);
    buffer_destroy(bufferEnviar);
    return data;
}

void enviar_fs_kernel(fs_kernel_data* data, int socket, kernel_fs_instruccion flag){
    t_buffer* bufferEnviar =  buffer_create();
    char* nombreArchivo = data->nombreArchivo;
    int offset = 0;
    offset = buffer_pack_string(bufferEnviar, nombreArchivo, offset);
    offset = buffer_pack(bufferEnviar, &data->pid, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &data->direcFisica, sizeof(int), offset);
    offset = buffer_pack(bufferEnviar, &data->puntero, sizeof(uint32_t), offset);
    offset = buffer_pack(bufferEnviar, &data->info, sizeof(uint32_t), offset);
    offset = buffer_pack(bufferEnviar, &data->tamanioNuevo, sizeof(int), offset);
    stream_send_buffer(socket, flag, bufferEnviar);
    buffer_destroy(bufferEnviar);
}

fs_kernel_data* recibir_fs_kernel(int socket){
    t_buffer* bufferEnviar =  buffer_create();
    fs_kernel_data* data = malloc(sizeof(fs_kernel_data));
    data->flag = stream_recv_header(socket);
    stream_recv_buffer(socket, bufferEnviar);
    int offset = 0;
    data->nombreArchivo = buffer_unpack_string(bufferEnviar, &offset);
    offset = buffer_unpack(bufferEnviar, &data->pid, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &data->direcFisica, sizeof(int), offset);
    offset = buffer_unpack(bufferEnviar, &data->puntero, sizeof(uint32_t), offset);
    offset = buffer_unpack(bufferEnviar, &data->info, sizeof(uint32_t), offset);
    offset = buffer_unpack(bufferEnviar, &data->tamanioNuevo, sizeof(int), offset);
    buffer_destroy(bufferEnviar);
    return data;
}


void liberarPcb(pcb *pcb1) {
    if (pcb1->registros_cpu != NULL) {
        free(pcb1->registros_cpu);
    }
    free(pcb1);
}
