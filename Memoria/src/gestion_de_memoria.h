#ifndef GESTION_DE_MEMORIA_H_
#define GESTION_DE_MEMORIA_H_

#include <commons/config.h>
#include<commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include<netdb.h>
#include<commons/log.h>
#include<buffer/buffer.h>
#include<server/server.h>
#include<structs/structs.h>
#include <commons/config.h>
#include <commons/config.h>
#include <utils/flags.h>
#include "configMemoria.h"
#include <time.h> 



//1 pagina 
typedef struct {
    int pid_pag;
    int numero_marco;
    bool bit_de_presencia;
    bool bit_modificado;
    uint32_t nro_bloque;
    time_t ultimo_acceso; //para LRU
} t_pagina;

//LRU ultimo acceso solo cuando leo/escribo

typedef struct {
    int nro_marco;
	int pid_pag;
    int pid_proceso;
    int orden; //para fifo
	bool libre;

} t_marco;


void* espacio_usuario;
t_list* tabla_de_paginacion;
t_dictionary* diccionario_tablas;
pthread_mutex_t mutex_orden;
t_list* marcos;

uint32_t leer_espacio_usuario(uint32_t direccion);
int buscar_pag_memoria(int pag, int pid);
t_pagina* buscar_pag_tabla(int pag, int proceso);
void escribir_en_mem(t_marco* marco, uint32_t valor);
t_pagina* buscar_marco_en_tabla_de_proceso(t_list* tabla,int nro_marco);
t_pagina* buscar_marco_en_tablas(t_marco* marco_victima);
void actualizar_tablas(t_marco* marco_victima, int presencia);
t_marco* reemplazar_marco_victima(int index);
int es_menos_usada();
int obtener_marco_victima();
void actualizar_pag_tabla(t_pagina* pag, int presencia);
void actualizar_marco(t_marco* marco);
void crear_tabla_de_paginacion(int cant_pag, int pid_proceso, t_list* bloques);
void crear_marcos_memoria(int cantidad_marcos);
bool esta_libre(t_marco* marco);
t_marco* buscar_marco_libre();
int cant_marcos();
void inicializar_memoria_principal();
void escribir_espacio_usuario(uint32_t direccion, uint32_t valor);
t_list* obtenerBloquesPid(int pid);
bool esta_libre(t_marco* marco);
bool esta_libre_cast(void* ptr);
bool hay_marcos_libres();



#endif
