#ifndef PETICIONES_KERNEL_H_
#define PETICIONES_KERNEL_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <utils/flags.h>
#include <structs/structs.h>
#include <string.h>
#include <buffer/send.h>
#include <main.h>

sem_t marcoExito;
sem_t escrituraExito;

void atenderKernelPeticiones();

void abrirArchivo(char* nombre_archivo);

void crearArchivo(char* nombre_archivo);

void truncarArchivo(fs_kernel_data* soliKernel);

void leerArchivo(fs_kernel_data* soliKernel);

void escribirArchivo(fs_kernel_data* soliKernel);

char* crearPathAlFCB(char* nombreArchivo);

uint32_t encontrar_bloque_n(char* nombre_archivo, uint32_t bloque_puntero);

uint32_t encontrar_proximo_bloque_libre_y_asignar_a_fat(uint32_t ultimo_bloque);

int encontrar_ultimo_bloque(uint32_t bloque_inicial);

uint32_t encontrar_proximo_bloque_libre();
#endif