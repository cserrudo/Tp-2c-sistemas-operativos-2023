#include "gestion_de_memoria.h"

extern memoria_config* memoriaConfig;
extern t_log* memoriaLogger;
pthread_mutex_t mutex_memoria;

bool esta_libre(t_marco* marco) {
    return marco->libre == true;
}

bool esta_libre_cast(void* ptr) {
	return esta_libre((t_marco*)ptr);
}

bool hay_marcos_libres(){
	return list_any_satisfy(marcos, esta_libre_cast);
}

void escribir_en_mem(t_marco* marco, uint32_t valor){
	escribir_espacio_usuario(marco->nro_marco * memoriaConfig->tam_pagina,valor); 
	t_pagina* pag_aux = buscar_marco_en_tablas(marco);
				
	pag_aux->ultimo_acceso = time(NULL);
}


t_pagina* buscar_marco_en_tabla_de_proceso(t_list* tabla,int pag){
	for(int i=0;i<list_size(tabla);i++){
		t_pagina* pagina = list_get(tabla,i);
		if(pagina->pid_pag == pag){
			return pagina;
		}
	}
	return NULL;
}

t_pagina* buscar_marco_en_tablas(t_marco* marco_victima){
	char* key = string_from_format("%d", marco_victima->pid_proceso);
	t_list* tabla = 
		dictionary_get(diccionario_tablas, key);
	if(tabla == NULL){
		log_warning(memoriaLogger, "no se encontro la tabla para la clave");
	}
	free(key);

	return buscar_marco_en_tabla_de_proceso(tabla, marco_victima->pid_pag);
}

void actualizar_tablas(t_marco* marco_victima, int presencia){
	t_list* tabla = 
		dictionary_get(diccionario_tablas, string_from_format("%d", marco_victima->pid_proceso));
	if(tabla == NULL){
		log_warning(memoriaLogger, "no se encontro la tabla para la clave");
	}
	t_pagina* pag = buscar_marco_en_tabla_de_proceso(tabla, marco_victima->pid_pag);

	pag->numero_marco = marco_victima->nro_marco;
	actualizar_pag_tabla(pag,presencia);

}


t_marco* reemplazar_marco_victima(int index){
	t_marco* marco_victima = list_get(marcos, index);
	t_pagina* pagina = buscar_marco_en_tablas(marco_victima);
	if(pagina->bit_modificado == 1){
		uint32_t data = leer_espacio_usuario(marco_victima->nro_marco * memoriaConfig->tam_pagina); 
		//swappeo mandando data y nro bloque
		log_info(memoriaLogger,"dato: %u", data);

		//me falta volver a cargar en la tabla maestra lo q cargue
		fs_memoria_data* soliFS = malloc(sizeof(*soliFS));

		soliFS->bloques_asignados = list_create();
		soliFS->numero_de_bloque = pagina->nro_bloque;
		soliFS->info = data;
		soliFS->cantEntradas = 0;
		soliFS->direcFisica = 0;
		soliFS->pid = 0;
		//soliFS->numero_de_bloque = marco-> aca tenemos que marcar q bloque es para fs
		enviar_fs_memoria(soliFS, memoriaConfig->socket_fs, i_escribir_swap);
		//TODO swappeo

	}
	pagina->bit_modificado = 0;
	pagina->bit_de_presencia = 0;
	pagina->numero_marco = -1;
	
	return marco_victima;
}
/*
void asignar_bloques_a_paginas(int bloques){
	int bloque; // aca le pido a FS la lista de bloques
	for(int i=0;i<list_size(tabla_de_paginacion);i++){
		t_pagina* pagina = list_get(tabla_de_paginacion,i);
		pagina->nro_bloque = bloque;
	}
}*/

int es_menos_usada(){
	int indice_menos_usada = 0;

    t_marco* marco_menor_ultimo_acceso = list_get(marcos,0);
	t_pagina* pag_menor_ultimo_acceso = buscar_marco_en_tablas(marco_menor_ultimo_acceso);


    for (int i = 1; i < list_size(marcos); i++) {
		t_marco* marco_aux = list_get(marcos,i);
		t_pagina* pagina_aux = buscar_marco_en_tablas(marco_aux);

        if (pagina_aux->ultimo_acceso < pag_menor_ultimo_acceso->ultimo_acceso) {
            marco_menor_ultimo_acceso = marco_aux;
            indice_menos_usada = i;
        }
    }

    return indice_menos_usada;
}

bool comparador_orden(void* elem1, void* elem2) {
    t_marco* marco1 = (t_marco*)elem1;
    t_marco* marco2 = (t_marco*)elem2;
    
    return marco1->orden < marco2->orden;
}

int buscar_orden_menor(){
	list_sort(marcos, comparador_orden);
    
	for(int i=0;i<list_size(marcos);i++){
		t_marco* marco_aux = list_get(marcos,i);
		if(marco_aux->orden >=0){
			return i;
		}
	}

	return -1;
}

int obtener_marco_victima(){
	int index_marco;

	if(strcmp(memoriaConfig->algoritmo_reemplazo,"FIFO")==0){
		index_marco = buscar_orden_menor(); //busca el menor para fifo
	}else if(strcmp(memoriaConfig->algoritmo_reemplazo,"LRU")==0){
		index_marco = es_menos_usada();
	}else{
		log_info(memoriaLogger, "No es un algoritmo valido");
	}
	return index_marco;
}

void actualizar_pag_tabla(t_pagina* pag, int presencia){
	//se reinician
	pag->bit_modificado = 0;
	pag->bit_de_presencia = presencia;
	pag->numero_marco = -1;
}

void actualizar_marco(t_marco* marco){
	marco->libre=1;
	marco->pid_pag=0;
	marco->pid_proceso=0;
}

void liberar_memoria_principal(){

  dictionary_destroy(diccionario_tablas); 

  pthread_mutex_destroy(&mutex_memoria); 
  pthread_mutex_destroy(&mutex_orden);

  free(espacio_usuario);
  list_destroy_and_destroy_elements(marcos, free);

}


t_pagina* buscar_pag_tabla(int pag, int proceso){
	char* key = string_from_format("%d", proceso);
	t_list* tabla = list_create();
	tabla = (t_list*)dictionary_get(diccionario_tablas, key);
	if(tabla == NULL){
		log_warning(memoriaLogger, "no se encontro la tabla para la clave");
	} else {
		for(int i=0;i<list_size(tabla);i++){
			t_pagina* pagina_aux = list_get(tabla, i);
			if(pagina_aux->pid_pag == pag){
				return pagina_aux;
			}
		}
	}
	return NULL;
}

static bool esta_en_mem(t_marco *marco,int pag, int pid) {
	return marco->pid_pag == pag && marco->pid_proceso== pid;  
}

int buscar_pag_memoria(int pag, int pid){
	for(int i=0;i<list_size(marcos);i++){
		t_marco* marco_aux = list_get(marcos,i);
		if(esta_en_mem(marco_aux,pag,pid))
			return i;
	}
	return -1;
}

t_list* obtenerBloquesPid(int pid){
	t_list* listaBloquesRta = list_create();
	char* clave = string_from_format("%d", pid);
	t_list* tabla = dictionary_get(diccionario_tablas, clave);
	for(int i=0; i<list_size(tabla);i++){
		t_pagina *pag = list_get(tabla, i);
		list_add(listaBloquesRta,pag->nro_bloque);
	}
	return listaBloquesRta;
}






void crear_tabla_de_paginacion(int cant_pag, int pid_proceso, t_list* bloques) {
	t_list* listaNueva = list_create();
	for(int i = 0; i < cant_pag; i++) {
		t_pagina *pag = malloc(sizeof(*pag));
		pag->pid_pag = i;
		pag->bit_de_presencia = 0;
		pag->nro_bloque = list_get(bloques,i);

		list_add(listaNueva, pag);
	}

	char* clave = string_from_format("%d", pid_proceso);
	dictionary_put(diccionario_tablas, clave, (t_list*)listaNueva);
	/*t_list *tabla = dictionary_get(diccionario_tablas, clave);
	t_pagina *pag2  = list_get(tabla, 0);*/
	

	free(clave);  
}
/*
void agregar_a_tablas(t_list* tabla_de_paginacion, int pid_proceso){
	t_tablas *tabla = malloc(sizeof(t_tablas));
	tabla->pid_proceso = pid_proceso;
	tabla->tabla_de_paginas = tabla_de_paginacion;

	list_add(tablas_de_paginacion,tabla);
}*/


void crear_marcos_memoria(int cantidad_marcos) {
	for(int i = 0; i < cantidad_marcos; i++) {
		t_marco* marco = malloc(sizeof(* marco));

		marco->pid_pag = -1;
		marco->nro_marco = i;
		marco->libre = 1;
		marco->pid_proceso = -1;
		marco->orden = -1;
		list_add(marcos, marco);
	}
}


t_marco* buscar_marco_libre(){
	for(int i = 0; i <list_size(marcos); i++){
		if(esta_libre(list_get(marcos,i))){
			return list_get(marcos,i);
		}
	}
	return NULL;

}

int cant_marcos(){
	return memoriaConfig->tam_memoria / memoriaConfig->tam_pagina;
}

void inicializar_memoria_principal(){
	espacio_usuario = malloc(memoriaConfig->tam_memoria);

	memset(espacio_usuario,0,memoriaConfig->tam_memoria); //inicializa con 0

	marcos = list_create();
	diccionario_tablas = dictionary_create();

	pthread_mutex_init(&mutex_memoria, NULL);
	pthread_mutex_init(&mutex_orden, NULL);

	crear_marcos_memoria(cant_marcos());

}

uint32_t leer_espacio_usuario(uint32_t direccion) {
	uint32_t valor;

	pthread_mutex_lock(&mutex_memoria);
	memcpy(&valor, espacio_usuario + direccion, sizeof(uint32_t));
	pthread_mutex_unlock(&mutex_memoria);

	return valor;
}

void escribir_espacio_usuario(uint32_t direccion, uint32_t valor) {
	pthread_mutex_lock(&mutex_memoria);
	memcpy(espacio_usuario + direccion, &valor, sizeof(int));
	pthread_mutex_unlock(&mutex_memoria);
}
