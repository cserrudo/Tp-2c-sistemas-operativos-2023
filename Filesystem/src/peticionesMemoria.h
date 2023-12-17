#ifndef PETICIONES_MEMORIA_H_
#define PETICIONES_MEMORIA_H_
#include <commons/log.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <utils/flags.h>
#include <buffer/send.h>
//#include <buffer/buffer.h>
#include <buffer/buffer.h>
#include <main.h>

t_list* infoDesdeMemoria;

void atenderMemoriaPeticiones();

void iniciarProceso(fs_memoria_data* soliMemoria);

void finalizarProceso(fs_memoria_data* soliMemoria);

uint32_t buscarBloqueLibreSwap();

void reservarBloque(uint32_t bloque_libre);

void liberarBloque(uint32_t);

void inicializarBitArray();

void atenderPageFault(fs_memoria_data* soliMemoria);

void escribirSwap(fs_memoria_data* soliMemoria);

#endif