#include <stdlib.h>
#include <stdio.h>
#include <configFS.h>
#include <peticionesMemoria.h>
#include <server/server.h>
#include <sys/mman.h>

extern fs_config* fsConfig;
extern t_log* fsLogger;
extern void* mapeo_swap;
t_bitarray* bitarray_bloques;
extern sem_t marcoExito;
extern sem_t escrituraExito;
uint32_t infoAgregar;


void atenderMemoriaPeticiones(){
    inicializarBitArray();
    infoDesdeMemoria = list_create();
    while(1){
        //recibir de memoria peticion
        //memoria_fs_instruccion flag_instruccion = stream_recv_header(fsConfig->socket_memoria);
        fs_memoria_data* soliMemoria = recibir_fs_memoria(fsConfig->socket_memoria);
        log_info(fsLogger, "se recibio nueva peticion de memoria");
        switch(soliMemoria->flag){
            case i_iniciar_proceso:
                //recibe todo en iniciarProceso
                
                pthread_t iniciar_proceso;
	            pthread_create(&iniciar_proceso, NULL, (void*) iniciarProceso, soliMemoria);
	            pthread_detach(iniciar_proceso);            
            break;
            case i_finalizar_proceso:
           //recibir info de memoria(checkpoint 3)

                pthread_t finalizar_proceso;
	            pthread_create(&finalizar_proceso, NULL, (void*) finalizarProceso, soliMemoria);
	            pthread_detach(finalizar_proceso);          
            break;
            case i_page_fault:
                pthread_t atender_page_fault;
	            pthread_create(&atender_page_fault, NULL, (void*) atenderPageFault, soliMemoria);
	            pthread_detach(atender_page_fault);

            break;
            case i_escribir_swap:
                pthread_t escribir_swap;
	            pthread_create(&escribir_swap, NULL, (void*) escribirSwap, soliMemoria);
	            pthread_detach(atender_page_fault);
            break;
            case MARCO_EXITO_FS:
                log_info(fsLogger, "se recibio  marco exito");
               
                sem_post(&marcoExito);
            break;
            case ESCRITURA_EXITO_FS:
            infoAgregar = soliMemoria->info;
            sem_post(&escrituraExito);
            break;
            default:
            log_error(fsLogger, "No existe la instruccion para ser atendida por filesystem, pedida de Memoria");
            break;            
        }
    }

}

void escribirSwap(fs_memoria_data* soliMemoria){
    uint32_t numero_de_bloque = 0;//recibimos numero de pagina(se nos va a pasar el bloque en el que esta)    
    numero_de_bloque =  soliMemoria->numero_de_bloque;
    log_info(fsLogger, "Acceso SWAP dato : %u", soliMemoria->info);
    memcpy(mapeo_swap + (numero_de_bloque*fsConfig->tam_bloque), &(soliMemoria->info), fsConfig->tam_bloque);//escribimos en bloque la info
	msync(mapeo_swap,  fsConfig->cant_bloques_swap * fsConfig->tam_bloque, MS_SYNC);	
    log_info(fsLogger, "Acceso SWAP: %u", numero_de_bloque);
    usleep(fsConfig->retardo_acceso_bloque);
    //free(info);//nose si va o no, depende como plantiemos
}

void atenderPageFault(fs_memoria_data *soliMemoria) {
	uint32_t numero_de_bloque = 0; //recibimos numero de pagina(se nos va a pasar el bloque en el que esta)
	numero_de_bloque = soliMemoria->numero_de_bloque;
	uint32_t *info = malloc(fsConfig->tam_bloque);
	if (bitarray_test_bit(bitarray_bloques, numero_de_bloque)) {
		memcpy(info, mapeo_swap + (numero_de_bloque * fsConfig->tam_bloque),
				fsConfig->tam_bloque); //buscamos la info que hay en ese bloque
		log_info(fsLogger, "Acceso SWAP: %u", numero_de_bloque);
		usleep(fsConfig->retardo_acceso_bloque);
	} else {
		*info = 0; //si esta vacio, devuevlo un 0 de vacio
	}

	//se lo mandamos a memoria
	soliMemoria->info = *info;
	log_info(fsLogger, "enviando el valor info %d", *info);
	enviar_fs_memoria(soliMemoria, fsConfig->socket_memoria, i_page_fault);
	free(info);
}

void iniciarProceso(fs_memoria_data* soliMemoria){
    int pid,cant_entradas;
    cant_entradas = soliMemoria->cantEntradas;
    pid = soliMemoria->pid;

    t_list* bloques_asignados = list_create();
    int bloques_a_reservar= cant_entradas;//llenar con info de memoria
    //llenar y guardar bloques reservados
    for(int i=0;i<bloques_a_reservar;i++){
        uint32_t bloque_libre = buscarBloqueLibreSwap();
        reservarBloque(bloque_libre);
        log_info(fsLogger, "Acceso SWAP: %u", bloque_libre);
        usleep(fsConfig->retardo_acceso_bloque);
        list_add(bloques_asignados, bloque_libre);
    }

    soliMemoria->bloques_asignados = bloques_asignados;
    enviar_fs_memoria(soliMemoria, fsConfig->socket_memoria, i_iniciar_proceso);

    //enviar a memoria los bloques que fueron reservados
}

void finalizarProceso(fs_memoria_data* soliMemoria){
    log_info(fsLogger, "liberando bloques del proceso");
    t_list* bloques_a_liberar = list_create();
   bloques_a_liberar = soliMemoria->bloques_asignados;
    //recibir info memoria
    //marcar como libres los bloques solicitados
    for(int i=0; i < list_size(bloques_a_liberar);i++){
        uint32_t bloque_a_liberar = list_get(bloques_a_liberar, i);
        liberarBloque(bloque_a_liberar);
        log_info(fsLogger, "Acceso SWAP: %u", bloque_a_liberar);
        usleep(fsConfig->retardo_acceso_bloque);
    }
    log_info(fsLogger, "se libero con exito, mando la liberacion de memoria");
     
    enviar_fs_memoria(soliMemoria, fsConfig->socket_memoria, finalizacion_ok_fs);
}

uint32_t buscarBloqueLibreSwap(){
    uint32_t i = 0; 
    while(bitarray_test_bit(bitarray_bloques, i)){
        //printf("%d ", bitarray_test_bit(bitarray_bloques, i));
        i++;
    }
    return i;
}

void inicializarBitArray(){
    void* bitarray_aux = malloc(fsConfig->cant_bloques_swap);
    bitarray_bloques = bitarray_create_with_mode(bitarray_aux, fsConfig->cant_bloques_swap, LSB_FIRST);  
    for(int i=0; i<fsConfig->cant_bloques_swap;i++){
        bitarray_clean_bit(bitarray_bloques, i);
    }
    /* para chequear si asigna bien, lo hace
    for(int i = 0; i < fsConfig->cant_bloques_swap; i++){
        printf("%d ", bitarray_test_bit(bitarray_bloques, i));
    }
    printf("\n");*/
}

void reservarBloque(uint32_t bloque_libre){
    //llenar con \0
    //char* aux = '\0';
    char* aux = malloc(fsConfig->tam_bloque); 
    memset(aux, '\0', fsConfig->tam_bloque);
    for(int i=0; i<fsConfig->tam_bloque;i+=sizeof(char)){
        memcpy(mapeo_swap+(bloque_libre*fsConfig->tam_bloque)+i,aux,sizeof(char));
	    msync(mapeo_swap,  fsConfig->cant_bloques_swap * fsConfig->tam_bloque, MS_SYNC);
    }
    bitarray_set_bit(bitarray_bloques, bloque_libre);
    free(aux);
}
void liberarBloque(uint32_t bloque_a_liberar){
    uint32_t aux = 0;
    for(int i=0; i<fsConfig->tam_bloque;i+=sizeof(char)){
        memcpy(mapeo_swap+(bloque_a_liberar*fsConfig->tam_bloque)+i,&aux,sizeof(char));
	    msync(mapeo_swap,  fsConfig->cant_bloques_swap * fsConfig->tam_bloque, MS_SYNC);
    }
    bitarray_clean_bit(bitarray_bloques, bloque_a_liberar);
}
