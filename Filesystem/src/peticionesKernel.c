#include <stdlib.h>
#include <stdio.h>
#include <configFS.h>
#include <peticionesKernel.h>
#include <dirent.h>
#include <sys/types.h>
#include <server/server.h>
#include <sys/mman.h>

extern fs_config* fsConfig;
extern t_log* fsLogger;
extern void* mapeo_swap;
extern void* mapeo_tabla_fat;
extern void* mapeo_bloques_fat;
extern t_list* infoDesdeMemoria;
extern uint32_t infoAgregar;
sem_t bloque_inicial_creado;

char* nombre_archivo;
int nuevo_tamanio;
t_list* lista_vacia;
pthread_mutex_t mutex_bloque;

void atenderKernelPeticiones(){
    sem_init(&marcoExito, 0, 0);
    sem_init(&escrituraExito, 0, 0);
    sem_init(&bloque_inicial_creado, 0, 0);

    lista_vacia = list_create();
    while(1){
        //recibir de kernel peticion
        
        fs_kernel_data* soliKernel = recibir_fs_kernel(fsConfig->socket_kernel);
        switch(soliKernel->flag){
            case i_open:
                //recibir nombre del archivo
                
                nombre_archivo = soliKernel->nombreArchivo;

                pthread_t abrir_archivo;
	            pthread_create(&abrir_archivo, NULL, (void*) abrirArchivo, nombre_archivo);
	            pthread_detach(abrir_archivo);
            break;
            case i_create:
                //recibir nombre del archivo
                
                nombre_archivo = soliKernel->nombreArchivo;
                pthread_t crear_archivo;
	            pthread_create(&crear_archivo, NULL, (void*) crearArchivo, nombre_archivo);
	            pthread_detach(crear_archivo); 
                         
            break;
            case i_truncate:
                //recibir nombre del archivo
                pthread_t truncar_archivo;
	            pthread_create(&truncar_archivo, NULL, (void*) truncarArchivo, soliKernel);
	            pthread_detach(truncar_archivo);            
            break;
            case i_read:
                //recibir nombre del archivo
                // lo recibe en leer archivo
                pthread_t leer_archivo;
	            pthread_create(&leer_archivo, NULL, (void*) leerArchivo, soliKernel);
	           pthread_detach(leer_archivo);            
            break;
            case i_write:
                //recibir nombre del archivo
                pthread_t escribir_archivo;
	            pthread_create(&escribir_archivo, NULL, (void*) escribirArchivo, soliKernel);
	            pthread_detach(escribir_archivo);            
            break;
            default:
            log_error(fsLogger, "No existe la instruccion para ser atendida por filesystem, pedida de Kernel");
            break;
        }
    }
    
}

void abrirArchivo(char* nombre_archivo){
    log_info(fsLogger, "Abrir Archivo: %s", nombre_archivo);
    DIR* dir;
    struct dirent* ent;
    int tamanio = -1;

    dir = opendir(fsConfig->path_fcb);
    if(dir == NULL){
        log_error(fsLogger, "No se pudo abrir la carpeta de FCBs");
    }

    while((ent = readdir(dir)) != NULL){
        if(strstr(ent->d_name, nombre_archivo) != NULL){
            char* newPath = crearPathAlFCB(nombre_archivo);
            t_config* fcb_config= config_create(newPath);
            tamanio = config_get_int_value(fcb_config, "TAMANIO_ARCHIVO");
            config_destroy(fcb_config);
        }
    }
    closedir(dir);
    t_buffer* bufferRTA = buffer_create();
    fs_kernel_data *data = malloc(sizeof(*data));
    data->direcFisica = 0;
    data->info = 0;
    data->pid = 0;
    data->puntero=0;
    if(tamanio < 0){
        //enviar que no existe el archivo
        data->tamanioNuevo = 0;
        data->nombreArchivo = nombre_archivo;
        enviar_fs_kernel(data, fsConfig->socket_kernel, archivo_inexsitente);
    }
    else{
        //Enviar la variable tamanio
        data->nombreArchivo = "";
        data->tamanioNuevo = tamanio;
        enviar_fs_kernel(data, fsConfig->socket_kernel, creacion_ok);
    }
    buffer_destroy(bufferRTA);
}

void crearArchivo(char* nombre_archivo){
    log_info(fsLogger, "Crear Archivo: %s", nombre_archivo);
    char* newPath = crearPathAlFCB(nombre_archivo);
    FILE* f_fcb = fopen(newPath, "w");
    fclose(f_fcb);

    t_config* fcb_config= config_create(newPath);
    config_set_value(fcb_config, "NOMBRE_ARCHIVO", nombre_archivo);
    config_set_value(fcb_config, "TAMANIO_ARCHIVO", "0");
    config_save(fcb_config);
    config_destroy(fcb_config);

    fs_kernel_data *data = malloc(sizeof(*data));
    data->direcFisica = 0;
    data->info = 0;
    data->pid = 0;
    data->puntero=0;
    data->tamanioNuevo=0;
    data->nombreArchivo = nombre_archivo;
    enviar_fs_kernel(data, fsConfig->socket_kernel, creacion_ok);
}

void truncarArchivo(fs_kernel_data* soliKernel){

    nombre_archivo = soliKernel->nombreArchivo;
    int nuevo_tamanio = soliKernel->tamanioNuevo;
    int pid = soliKernel->pid;

    log_info(fsLogger, "Truncar Archivo: %s - Tamaño: %d", nombre_archivo, nuevo_tamanio);
    char* newPath = crearPathAlFCB(nombre_archivo);
    t_config* fcb_config= config_create(newPath);
    int tamanio_actual = config_get_int_value(fcb_config, "TAMANIO_ARCHIVO");
    if(nuevo_tamanio > tamanio_actual){
        int bloques_a_asignar = (nuevo_tamanio - tamanio_actual)/fsConfig->tam_bloque;
        log_info(fsLogger, "tamanio nuevo es > al tamanio anterior, chequeando si existe el bloque inicial");

        if(config_has_property(fcb_config, "BLOQUE_INICIAL")){
            uint32_t bloque_inicial = config_get_int_value(fcb_config, "BLOQUE_INICIAL");
            log_info(fsLogger, "bloque inicial existe y es de %u", bloque_inicial);
            int ultimo_bloque = encontrar_ultimo_bloque(bloque_inicial);
            for(int i=0; i<bloques_a_asignar; i++){
            ultimo_bloque = encontrar_proximo_bloque_libre_y_asignar_a_fat(ultimo_bloque);
            }
        }
        else{
            uint32_t bloque_inicial = encontrar_proximo_bloque_libre();
            log_info(fsLogger, "bloque inicial no existe, asignando el bloque libre %u como inicial", bloque_inicial);
            config_set_value(fcb_config, "BLOQUE_INICIAL", string_itoa((int)bloque_inicial));
            config_save(fcb_config);
            sem_post(&bloque_inicial_creado);
            for(int i=1; i<bloques_a_asignar;i++){
                bloque_inicial = encontrar_proximo_bloque_libre_y_asignar_a_fat(bloque_inicial);
            }
        }

    }else if(nuevo_tamanio < tamanio_actual){
        log_info(fsLogger, "tamanio nuevo es < al tamanio anterior, chequeando el nuevo tamanio");
        uint32_t bloque_inicial = config_get_int_value(fcb_config, "BLOQUE_INICIAL");
        int bloques_a_descartar = (tamanio_actual-nuevo_tamanio)/fsConfig->tam_bloque;
        uint32_t bloque_nulo = 0;
        if(nuevo_tamanio == 0){
            log_info(fsLogger, "truncando archivo a tamanio 0");
            for(int i=0; i<bloques_a_descartar;i++){
                int ultimo_bloque = encontrar_ultimo_bloque(bloque_inicial);
                memcpy(mapeo_tabla_fat + (ultimo_bloque * sizeof(uint32_t)), &bloque_nulo, sizeof(uint32_t));  
		msync(mapeo_tabla_fat, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);    
                log_info(fsLogger, "Acceso a FAT - Entrada: %d - Valor: %u ", ultimo_bloque, bloque_nulo);      
                usleep(fsConfig->retardo_acceso_fat);        
            }
            config_remove_key(fcb_config, "BLOQUE_INICIAL");
        }
        else{
            uint32_t bloque_max = UINT32_MAX;
            log_info(fsLogger, "truncando archivo bloques %d", bloques_a_descartar);
            int ultimo_bloque = -1;
            for(int i=0; i<bloques_a_descartar;i++){
                ultimo_bloque = encontrar_ultimo_bloque(bloque_inicial);
                memcpy(mapeo_tabla_fat+(sizeof(uint32_t)*ultimo_bloque),&bloque_nulo, sizeof(uint32_t));
		msync(mapeo_tabla_fat, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);
                log_info(fsLogger, "Acceso a FAT - Entrada: %d - Valor: %u ", ultimo_bloque, bloque_nulo);
                usleep(fsConfig->retardo_acceso_fat);
            } 
            memcpy(mapeo_tabla_fat+(sizeof(uint32_t)*(ultimo_bloque-1)),&bloque_max, sizeof(uint32_t));
	    msync(mapeo_tabla_fat, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);
            log_info(fsLogger, "Acceso a FAT - Entrada: %d - Valor: %u ", (ultimo_bloque - 1), bloque_max);
            usleep(fsConfig->retardo_acceso_fat);
        }
    }
    log_info(fsLogger, "finalizando trucado");
    config_set_value(fcb_config, "TAMANIO_ARCHIVO", string_itoa(nuevo_tamanio));
    config_save(fcb_config);
    config_destroy(fcb_config);
    log_info(fsLogger, "enviando a kernel la liberacion del proceso");
    enviar_fs_kernel(soliKernel, fsConfig->socket_kernel, ejecuta_ok);
    
}

void leerArchivo(fs_kernel_data* soliKernel){
    nombre_archivo = soliKernel->nombreArchivo;
    uint32_t puntero = soliKernel->puntero;
    int direccionFisica = soliKernel->direcFisica;
    int pid = soliKernel->pid;
    //esta df es para memoria
    log_info(fsLogger, "Leer Archivo: %s - Puntero: %d - Memoria: %d", nombre_archivo, puntero, direccionFisica);
    //leer la cantidad pedida de info a partir del puntero(siempre se lee el contenido completo del bloque)
    //cambiar por puntero del archivo
    uint32_t bloque_puntero = puntero/fsConfig->tam_bloque;
    uint32_t bloque_a_leer = encontrar_bloque_n(nombre_archivo, bloque_puntero);
    void* info = malloc(fsConfig->tam_bloque);
    memcpy(info, mapeo_bloques_fat+(bloque_a_leer*fsConfig->tam_bloque),fsConfig->tam_bloque);
    log_info(fsLogger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %u - Bloque FS: %u", nombre_archivo, bloque_puntero, bloque_a_leer);
    usleep(fsConfig->retardo_acceso_bloque);
    //mandar esa info a memoria, con la DF
    fs_memoria_data* soliMemoria = malloc(sizeof(*soliMemoria));
    soliMemoria->info = info;
    soliMemoria->bloques_asignados = lista_vacia;
    soliMemoria->pid = pid;
    soliMemoria->direcFisica=0;
    soliMemoria->numero_de_bloque=0;
    soliMemoria->cantEntradas = 0;
    uint32_t dataLeida = info;
  
    enviar_fs_memoria(soliMemoria, fsConfig->socket_memoria, ACCESO_A_ESPACIO_USUARIO_ESCRITURA);

    //este es el ok
    sem_wait(&marcoExito);
    log_info(fsLogger, "Se escribio con exito en memoria para la df %d el valor %u", direccionFisica, dataLeida);
    

    //pasar esto a peticiones memoria con un flag aca


    //recibir confirmacion de memoria
    enviar_fs_kernel(soliKernel, fsConfig->socket_kernel, ejecuta_ok);
      free(info);
}

void escribirArchivo(fs_kernel_data* soliKernel){
    pthread_mutex_init(&mutex_bloque, NULL);
    nombre_archivo = soliKernel->nombreArchivo;
    uint32_t puntero = soliKernel->puntero;//cambiar por puntero del archivo
    int direcFisica= soliKernel->direcFisica; //esto se lo manda a memoria
    int pid = soliKernel->pid;
    log_info(fsLogger, "Escribir Archivo: %s - Puntero: %d - Memoria: %d", nombre_archivo, puntero, direcFisica);
    //solicitar a memoria la info que se encuentra en la DF recibida por parametro(el tamanio debe coincidir con el del bloque)
    
    fs_memoria_data* soliMemoria = malloc(sizeof(*soliMemoria));
    soliMemoria->pid = pid;
    soliMemoria->direcFisica = direcFisica;
    soliMemoria->bloques_asignados = list_create();
    soliMemoria->numero_de_bloque=0;
    soliMemoria->cantEntradas = 0;
    soliMemoria->info = 0;
    enviar_fs_memoria(soliMemoria, fsConfig->socket_memoria, ACCESO_A_ESPACIO_USUARIO_LECTURA);

    sem_wait(&escrituraExito);
    //falta chequear lo del tamanio que coincidia

    void* info = &infoAgregar;
    
    //escribir la info en el bloque solicitado que se obtiene a partir del puntero recibido
    uint32_t bloque_puntero = puntero/fsConfig->tam_bloque;
    uint32_t bloque_a_escribir = encontrar_bloque_n(nombre_archivo, bloque_puntero);
    pthread_mutex_lock(&mutex_bloque);
    memcpy(mapeo_bloques_fat+(bloque_a_escribir*fsConfig->tam_bloque),info,fsConfig->tam_bloque);
    msync(mapeo_bloques_fat, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * fsConfig->tam_bloque, MS_SYNC);
    pthread_mutex_unlock(&mutex_bloque);
    log_info(fsLogger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %u - Bloque FS: %u", nombre_archivo, bloque_puntero, bloque_a_escribir); 
    usleep(fsConfig->retardo_acceso_bloque);   
    enviar_fs_kernel(soliKernel, fsConfig->socket_kernel, ejecuta_ok);
}

char* crearPathAlFCB(char* nombreArchivo){
    char* newPath = string_new();
    string_append(&newPath, fsConfig->path_fcb);
    string_append(&newPath, "/");
    string_append(&newPath, nombreArchivo);
    string_append(&newPath, ".fcb");
    return newPath;
}

uint32_t encontrar_proximo_bloque_libre_y_asignar_a_fat(uint32_t ultimo_bloque){
    uint32_t bloque_max = UINT32_MAX;
    uint32_t bloque_libre = encontrar_proximo_bloque_libre();
    memcpy(mapeo_tabla_fat + (sizeof(uint32_t)*ultimo_bloque), &bloque_libre, sizeof(uint32_t));
msync(mapeo_tabla_fat, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);	
    log_info(fsLogger, "Acceso a FAT - Entrada: %u - Valor: %u ", ultimo_bloque, bloque_libre);
    usleep(fsConfig->retardo_acceso_fat);
    memcpy(mapeo_tabla_fat + (sizeof(uint32_t) * bloque_libre), &bloque_max, sizeof(uint32_t));
	msync(mapeo_tabla_fat, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);
    log_info(fsLogger, "Acceso a FAT - Entrada: %u - Valor: %u ", bloque_libre, bloque_max);
    usleep(fsConfig->retardo_acceso_fat);
    return bloque_libre;
}

int encontrar_ultimo_bloque(uint32_t bloque_inicial){
    int i = 0;
    while(bloque_inicial != UINT32_MAX){
        uint32_t bloque_anterior = bloque_inicial;
        memcpy(&bloque_inicial,mapeo_tabla_fat+ (bloque_inicial*sizeof(uint32_t)),sizeof(uint32_t));
        log_info(fsLogger, "Acceso a FAT - Entrada: %u - Valor: %u ", bloque_anterior, bloque_inicial);
        usleep(fsConfig->retardo_acceso_fat);
        i++;
    }
    return i;
}

uint32_t encontrar_proximo_bloque_libre(){
    uint32_t i=0;
    uint32_t bloque_libre = 1;
    uint32_t bloque_max = UINT32_MAX;
    
    while(bloque_libre != 0){
        i++;
        memcpy(&bloque_libre, mapeo_tabla_fat+(i*sizeof(uint32_t)), sizeof(uint32_t));    
        log_info(fsLogger, "Acceso a FAT - Entrada: %d - Valor: %u ", i, bloque_libre);
        usleep(fsConfig->retardo_acceso_fat);
        // printf("i: %u, bloque_libre: %u\n", i, bloque_libre);    
    }
    memcpy(mapeo_tabla_fat + (sizeof(uint32_t) * i), &bloque_max, sizeof(uint32_t));
	msync(mapeo_tabla_fat, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);
    log_info(fsLogger, "Acceso a FAT - Entrada: %d - Valor: %u ", i, bloque_max);
    usleep(fsConfig->retardo_acceso_fat);
    return i;
}

uint32_t encontrar_bloque_n(char* nombre_archivo, uint32_t bloque_puntero){
    char* newPath = crearPathAlFCB(nombre_archivo);
    t_config* fcb_config= config_create(newPath);
    if(!config_has_property(fcb_config, "BLOQUE_INICIAL")){
        sem_wait(&bloque_inicial_creado);
    }
    uint32_t bloque_aux = config_get_int_value(fcb_config, "BLOQUE_INICIAL");
    for(int i=0; i<bloque_puntero;i++){
        uint32_t bloque_anterior = bloque_aux;
        memcpy(&bloque_aux, mapeo_tabla_fat + (bloque_aux*sizeof(uint32_t)),sizeof(uint32_t));
        log_info(fsLogger, "Acceso a FAT - Entrada: %u - Valor: %u ", bloque_anterior, bloque_aux);
        usleep(fsConfig->retardo_acceso_fat);
    }
    config_destroy(fcb_config);
    return bloque_aux;
}

