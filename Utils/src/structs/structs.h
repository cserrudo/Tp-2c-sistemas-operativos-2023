#ifndef STRUCTS_H
#define STRUCTS_H
#include "../utils/flags.h"
#include <commons/collections/list.h>
#include <pthread.h>
#include <stdint.h>

//estructuras communes entre los distintos modulos


//regristros_cpu usado por cpu y kernel(en pcb)
typedef struct {

	uint32_t AX, BX, CX, DX;
}registros_cpu;

typedef struct{
	int pid;
	int programCounter;
	int prioridad;
	registros_cpu* registros_cpu;
	t_list* tabla_archivos;
	kernel_cpu_dato flag;
	int sleep;
	char* recurso;
	char* archivo;
	int puntero_size; //este es usado tanto para f_truncate y f_seek
	uint32_t direccion;
	char *status;
	char *resource_requested;
	char *file_requested;
	t_list *resources_taken;
}pcb; //usado entre kernel y cpu






typedef struct{
	char* nombre;
	int size;
	int pid; //el pid asociado a espacio de memoria
	int pagina;
	client_flag flag;
}kernel_memoria_data;


typedef struct{
	int pid;
	int cantEntradas;
	int direcFisica;
	memoria_fs_instruccion flag;
	t_list* bloques_asignados;
	uint32_t numero_de_bloque;
	uint32_t info;
} fs_memoria_data;

typedef struct{
	int pid;
	char* nombreArchivo;
	int direcFisica;
	kernel_fs_instruccion flag;
	int tamanioNuevo;
	int32_t puntero;
	int32_t info;
} fs_kernel_data;

typedef struct{
	//campos de instruccion
	int pid;
	int programCounter;
	char* param1;
	char* param2;
	char* param3;
	int direccion;
	uint32_t registroValor;

	client_flag flag;
}memoria_cpu_data;



//identificadores de instrucciones
typedef enum{
	SET,
	SUM,
	SUB,
	MOV_IN,
	MOV_OUT,
	SLEEP,
	JNZ,
	WAIT,
	SIGNAL,
	F_OPEN,
	F_TRUNCATE,
	F_SEEK,
	F_WRITE,
	F_READ,
	F_CLOSE,
	EXIT,
	PF,
}t_identificador;

//struct de instrucciones
typedef struct {
	t_identificador identificador;
	char* param1;
	char* param2;
	char* param3;
}t_instruccion;




typedef struct{
	char* nombre_archivo;
	uint32_t tamanio_archivo;
	uint32_t bloque_inicial;
}fcb;

typedef struct {
	
}fat;


#endif
