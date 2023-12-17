#include "planificador.h"
extern kernel_config *kernelConf;
extern t_log *kernelLogger;



#define cantida_procesos 10
#define CANTIDAD_RECURSOS 10
extern t_queue *arrayRecursos[CANTIDAD_RECURSOS];

// semaforos

int grado_actual;

int prioridadActual=-1;

bool cpu_ejecutando;

extern bool finProcesoEjecutando;

bool empezoExecute = false;

bool condicionPF;

bool pcbInicial;

bool condicionPrioridad;

bool desalojadoAntes;

int prioridadProcesoActual = 0;

pcb* pcbPrioridad;

pcb *procesoEjecutandoActual;

int numeroproceso = 0;

int quantumActual = 0;
int pidActual = 1;

// cantidad de recursos
int tamanioArrayRecurso = 0;

t_list *lista_punteros_archivos;
t_list *lista_espera_archivos;
t_list *lista_global_archivos;

pthread_t th_largo_plazo;
pthread_t th_execute_pcb;
pthread_t th_corto_plazo;
pthread_t th_fs_respuesta;
pthread_t th_memoria_respuesta;

sem_t interruptFunco;



void *nuevoProceso(int prioridad, int size, char *path)
{
    //pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "Creando nuevo pcb");
    //pthread_mutex_unlock(&mutexLoggerKernel);
    pcb *aux = malloc(sizeof(*aux));
    aux->pid = next_pid(); // crear logica para poner pid nuevo
    //pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "se genero el pid");
    //pthread_mutex_unlock(&mutexLoggerKernel);
    aux->programCounter = 0;
    aux->tabla_archivos = list_create(); // usado en Filesystem
    aux->prioridad = prioridad;
    aux->registros_cpu = (registros_cpu *)malloc(sizeof(registros_cpu));
    aux->registros_cpu->DX = 0;
    aux->registros_cpu->AX = 0;
    aux->registros_cpu->BX = 0;
    aux->registros_cpu->CX = 0;
    aux->archivo = "archivo_default";
    aux->direccion = 0;
    aux->flag = 0;
    aux->puntero_size = -1;
    aux->recurso = "recurso_default";
    aux->status = "NEW";
    aux->resource_requested = "recurso_default";
    aux->file_requested = "archivo_default";
    aux->resources_taken = list_create();

    crear_memoria_pcb(aux->pid, path, size);
    pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "Se crea el proceso ID %d en NEW", aux->pid);
    pthread_mutex_unlock(&mutexLoggerKernel);

    pthread_mutex_lock(&mutexNew);
    list_add(new, aux);
    pthread_mutex_unlock(&mutexNew);
    sem_post(&semNew);
}

int next_pid()
{
    numeroproceso++;
    return numeroproceso;
}

void iniciar_planificacion()
{
    desalojoexecute = false;
    new = list_create();
    ready = list_create();
    exec = list_create();
    exit_estado = list_create();
    blocked = list_create();
    deadlocks = list_create();

    // semaforos
    sem_init(&semNew, 0, 0);
    sem_init(&grado_multiprogramacion, 0, kernelConf->grado_max_multiprogramacion);
    sem_init(&semReady, 0, 0);
    sem_init(&semExecute, 0, 0);
    sem_init(&execute_libre, 0, 1);
    sem_init(&io, 0, 0);
    sem_init(&fCreate, 0, 0);
    sem_init(&interruptFunco, 0,0);
    pthread_mutex_init(&mutex_tabla_archivos, NULL);
    pthread_mutex_init(&lista_recursos, NULL);
    pthread_mutex_init(&mutex_quantum, NULL);
    pthread_mutex_init(&mutex_pid_global, NULL);
    pthread_mutex_init(&mutexNodo, NULL);

    // mutex estados
    pthread_mutex_init(&mutexBlocked, NULL);
    pthread_mutex_init(&mutexNew, NULL);
    pthread_mutex_init(&mutexReady, NULL);
    pthread_mutex_init(&mutexExec, NULL);
    pthread_mutex_init(&mutexExit, NULL);
    pthread_mutex_init(&mutexPrioridades, NULL);

    lista_espera_archivos = list_create();
    lista_global_archivos = list_create();

    // inicio el array de recursos
    

    
    pthread_create(&th_largo_plazo, NULL, (void *)largo_plazo, NULL);
    pthread_detach(th_largo_plazo);

    
    pthread_create(&th_execute_pcb, NULL, (void *)execute, NULL);
    pthread_detach(th_execute_pcb);

    pthread_create(&th_corto_plazo, NULL, (void *)corto_plazo, NULL);
    pthread_detach(th_corto_plazo);

    pthread_create(&th_fs_respuesta, NULL, (void *)desbloquearPcbFS, NULL);
    pthread_detach(th_fs_respuesta);

    pthread_create(&th_memoria_respuesta, NULL, (void *)desbloquearPcbMemoria, NULL);
    pthread_detach(th_memoria_respuesta);
}

void inicio_array_recursos(){
    sem_init(&planificacionPausada, 0, 1);
    tamanioArrayRecurso = sizeof(kernelConf->recurso);
    for (int i = 0; i < tamanioArrayRecurso; i++)
    {
        t_queue *cola = queue_create();
        arrayRecursos[i] = cola;
    }
}

void destruirPlani(){
    list_destroy(new);
    list_destroy(ready);
    list_destroy(exec);
    list_destroy(exit_estado);
    list_destroy(blocked);
    list_destroy(deadlocks);

    sem_destroy(&grado_multiprogramacion);
	sem_destroy(&semNew);
	sem_destroy(&semReady);
	sem_destroy(&semExecute);
	sem_destroy(&execute_libre);
	sem_destroy(&io);
    sem_destroy(&planificacionPausada);
	sem_destroy(&fCreate);
	pthread_mutex_destroy(&mutex_tabla_archivos);
	pthread_mutex_destroy(&lista_recursos);
    pthread_mutex_destroy(&mutex_quantum);
    pthread_mutex_destroy(&mutex_pid_global);
    pthread_mutex_destroy(&mutexNodo);
    pthread_mutex_destroy(&mutexBlocked);
    pthread_mutex_destroy(&mutexNew);
    pthread_mutex_destroy(&mutexReady);
    pthread_mutex_destroy(&mutexExec);
    pthread_mutex_destroy(&mutexExit);
    
    list_destroy(lista_espera_archivos);
    list_destroy(lista_global_archivos);

    pthread_cancel(&th_largo_plazo);

    pthread_cancel(&th_execute_pcb);
    

    pthread_cancel(&th_execute_pcb);
    
    pthread_cancel(&th_fs_respuesta);
    
    pthread_cancel(&th_memoria_respuesta);
    
}

void largo_plazo()
{

    pcbPrioridad = malloc(sizeof(pcb));
   
    while (1)
    {
        sem_wait(&semNew);
        sem_wait(&grado_multiprogramacion);
        sem_wait(&planificacionPausada);
        sem_post(&planificacionPausada);

        pthread_mutex_lock(&mutexNew);
        pcb *pcbReady = pop_estado(new);
        pthread_mutex_unlock(&mutexNew);

        pthread_mutex_lock(&mutexReady);
        push_estado(ready, pcbReady);
        pthread_mutex_unlock(&mutexReady);

        cambio_de_estado(pcbReady, "READY");
        logger_ready(kernelConf->algoritmo_planificacion);

        if (strcmp(kernelConf->algoritmo_planificacion, "PRIORIDADES") == 0 && procesoConMayorPrioridadChequeo(ready, prioridadProcesoActual) && empezoExecute)
        {
            
            desalojarPrioridad(-1);
        }
    
        else{
            pcbInicial = true;
            sem_post(&semReady);
            if(desalojoexecute){
                sem_post(&execute_libre);
                desalojoexecute = false;
            }
        }
        

        
        
    }
}

pcb* desalojarPrioridad(int cantidadDeReady){
        pcb* pcbExecute;
        pcbInicial = false;
            pcbExecute = procesoConMayorPrioridad(ready, prioridadProcesoActual);
            if (-1 != pcbExecute->pid)
            {
                enviar_interrupt_cpu_kernel(kernelConf->socket_cpu_interrupt, desalojo_prioridad);
                log_info(kernelLogger, "proceso entrante mayor prioridad, desalojando...");
                desalojadoAntes = false;
                condicionPrioridad = true;
                /*if(cantidadDeReady ==1){
                    sem_post(&semExecute);
                }*/
                sem_post(&semReady);
                sem_wait(&interruptFunco);

        
                pthread_mutex_lock(&mutexExec);
                push_estado(exec, pcbExecute);
                pthread_mutex_unlock(&mutexExec);
                prioridadActual = pcbExecute->pid;
                pcbPrioridad = pcbExecute;

                cambio_de_estado(procesoEjecutandoActual, "EXECUTE");
                sem_post(&semExecute);

                // muetx
            }
            sleep(1);
            return pcbExecute;
}


void corto_plazo()
{
    desalojadoAntes = true;
    pcbInicial = true;
    pcb *pcbExecute;

    while (1)
    {
        sem_wait(&semReady);
        sem_wait(&planificacionPausada);
        sem_post(&planificacionPausada);
       

        sem_wait(&execute_libre);

        if(!list_is_empty(ready) && desalojadoAntes){
            // se elige el pcb segun el algoritmo utilizado
            if (strcmp(kernelConf->algoritmo_planificacion, "FIFO") == 0)
            {
                pthread_mutex_lock(&mutexReady);
                pcbExecute = pop_estado(ready);
                pthread_mutex_unlock(&mutexReady);
            }
            if (strcmp(kernelConf->algoritmo_planificacion, "PRIORIDADES") == 0 )
            {   
                
                pcbExecute = prioridades(ready);
                prioridadActual = pcbExecute->pid;
                prioridadProcesoActual = pcbExecute->prioridad;
            }
            
            if (strcmp(kernelConf->algoritmo_planificacion, "RR") == 0)
            {
                pthread_mutex_lock(&mutexReady);
                pcbExecute = pop_estado(ready);
                pthread_mutex_unlock(&mutexReady);

                // creo un thread que empieza el quantum

                pthread_t th_quantum_rr;
                pthread_create(&th_quantum_rr, NULL, (void *)rr_quantum_start, (void *)quantumActual);
                pthread_detach(th_quantum_rr);
                // mutex
            }

            pthread_mutex_lock(&mutexExec);
            push_estado(exec, pcbExecute);
            pthread_mutex_unlock(&mutexExec);
            sem_post(&semExecute);
        }
        else{
            log_info(kernelLogger, "reubicando semaphoros, se desalojo un pcb en execute...");
            desalojadoAntes = true;
            sem_post(&semReady);
        }

    }
}

void execute()
{
    lista_punteros_archivos = list_create();
    pcb* pcbAuxError;
    
    while (1)
    {
        sem_wait(&planificacionPausada);
        sem_post(&planificacionPausada);
        sem_wait(&semExecute);

        
        pthread_mutex_lock(&mutexExec);
        pcb *pcbViejo = list_get(exec, 0);
        pthread_mutex_unlock(&mutexExec);

        
        pthread_mutex_lock(&mutex_pid_global);
        pidActual = pcbViejo->pid;
        procesoEjecutandoActual = pcbViejo;
        pthread_mutex_unlock(&mutex_pid_global);
        //pthread_mutex_lock(&mutexLoggerKernel);
        log_info(kernelLogger, "ejecutando el proceso con pid: %d", pcbViejo->pid);
        //pthread_mutex_unlock(&mutexLoggerKernel);
        enviar_cpu_kernel(pcbViejo, kernelConf->socket_cpu_dispatch, execute_cpu);
        cpu_ejecutando = true;
        //pthread_mutex_lock(&mutexLoggerKernel);
        log_info(kernelLogger, "esperando respuesta de cpu");
        //pthread_mutex_unlock(&mutexLoggerKernel);
        pcb *pcbActual = recibir_cpu_kernel(kernelConf->socket_cpu_dispatch);
        pcbActual->status = "EXECUTE";
        sem_wait(&planificacionPausada);
        sem_post(&planificacionPausada);
        empezoExecute = true;
        if(prioridadActual != pcbActual->pid && strcmp(kernelConf->algoritmo_planificacion, "PRIORIDADES") == 0 ){
            bool errorPrioridad = false;
            for(int j = 0;j < list_size(exit_estado);j++){
                pcbAuxError = list_get(exit_estado,j);
                 if(pcbActual->pid ==  pcbAuxError->pid){
                    errorPrioridad = true;
                 }
            }

            for(int j = 0;j < list_size(ready)-1;j++){
                bool enready = false;
                pcbAuxError = list_get(ready,j);
                 if(prioridadActual ==  pcbAuxError->pid){
                    enready = true;
                 }

                 if(!enready){
                    push_estado(ready,pcbPrioridad);
                    sem_post(&semReady);
                 }
            }
            
            if(errorPrioridad){
                sem_post(&execute_libre);
            }
            else{
                push_estado(ready, pcbActual);
                sem_post(&execute_libre);
                sem_post(&semReady);
            }

            
           
        }
        else if (!list_is_empty(exec))
        {
            pthread_mutex_lock(&mutexExec);
            pop_estado(exec);
            pthread_mutex_unlock(&mutexExec);
            liberarPcb(pcbViejo);
            pthread_mutex_lock(&mutexExec);
            push_estado(exec, pcbActual);
            pthread_mutex_unlock(&mutexExec);
            pthread_mutex_lock(&mutex_pid_global);
            procesoEjecutandoActual = pcbActual;
            pthread_mutex_unlock(&mutex_pid_global);
            cpu_ejecutando = false;
            //pthread_mutex_lock(&mutexLoggerKernel);
            log_info(kernelLogger, "respuesta de cpu recibida para pcb: %d , numero de instruccion: %d", pcbActual->pid, pcbActual->programCounter);
            //pthread_mutex_unlock(&mutexLoggerKernel);

            switch (pcbActual->flag)
            {
            case i_sleep:
                // este pasa a blocked? o sigue en execute? pq capaz es un IO y listo
                pop_estado(exec);
                instruccion_en_sleep(pcbActual);
                sem_post(&execute_libre);

                break;

            case i_wait:
                pthread_mutex_lock(&lista_recursos);
                char *resultado = wait(pcbActual);
                if (strcmp(resultado, "no existe recurso") == 0)
                {
                    // pasa a exit
                    pthread_mutex_lock(&mutexExit);
                    push_estado(exit_estado, pcbActual);
                    pthread_mutex_unlock(&mutexExit);


                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec); // saco de execute al pcb
                    pthread_mutex_unlock(&mutexExec);

                    exit_pcb(pcbActual, exit_estado, "EXECUTE"); // manda el proceso completo a exit

                    pthread_mutex_unlock(&lista_recursos); // desbloqueo recursos
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);
                    sem_post(&execute_libre);
                    break;
                }
                else if (strcmp(resultado, "falta recursos") == 0)
                {
                    // pasa a blocked y se pone en cola del recurso

                    pthread_mutex_lock(&mutexBlocked);
                    push_estado(blocked, pcbActual); // cola de blocked del recurso
                    pthread_mutex_unlock(&mutexBlocked);


                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    cambio_de_estado(pcbActual, "BLOCKED");
                    detect_deadlock();

                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_info(kernelLogger, "se pasa pcb de pid %d a estado Blocked por faltante del recurso %s. Cuando haya elementos de dicho recurso, se sacara de blockeado a ready. ", pcbActual->pid, pcbActual->recurso);
                    log_info(kernelLogger, "PID: %d - Bloqueado por:  %s", pcbActual->pid, pcbActual->recurso);
                    //pthread_mutex_unlock(&mutexLoggerKernel);
                    pthread_mutex_unlock(&lista_recursos); // desbloqueo recursos
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);
                    sem_post(&execute_libre);

                    break;
                }
                else if (strcmp(resultado, "hay recursos suficiente") == 0)
                {
                    // se le asigna el recurso y continua el algoritmo
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_info(kernelLogger, "se le asigno 1 recurso del tipo %s al pcb de pid %d", pcbActual->recurso, pcbActual->pid);
                    //pthread_mutex_unlock(&mutexLoggerKernel);

                    pthread_mutex_unlock(&lista_recursos); // desbloqueo recursos
                    sem_post(&semExecute);
                }
                break;

            case i_signal:

                pthread_mutex_lock(&lista_recursos); // bloqueo el uso de recursos por otro proceso
                char *resultadoSignal = i_signals(pcbActual);
                if (strcmp(resultadoSignal, "no existe recurso") == 0 || strcmp(resultadoSignal, "max recursos") == 0)
                {
                    // pasa a exit
                    pthread_mutex_lock(&mutexExit);
                    push_estado(exit_estado, pcbActual);
                    pthread_mutex_unlock(&mutexExit);
                    
                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    exit_pcb(pcbActual, exit_estado, "EXECUTE"); // manda el proceso completo a exit

                    pthread_mutex_unlock(&lista_recursos); // desbloqueo recursos
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);
                    sem_post(&execute_libre);
                    break;
                }
                else if (strcmp(resultadoSignal, "recursos liberado") == 0)
                {
                    // se le asigna el recurso y continua el algoritmo
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_info(kernelLogger, "se libero 1 recurso del tipo %s que estaba asignado al pcb de pid %d", pcbActual->recurso, pcbActual->pid);
                    //pthread_mutex_unlock(&mutexLoggerKernel);

                    pthread_mutex_unlock(&lista_recursos); // desbloqueo recursos
                    sem_post(&semExecute);
                }
                break;

            case i_fopen_r:
                pthread_mutex_lock(&mutex_tabla_archivos);
                if (archivoModoAbiertoLista(lista_global_archivos, pcbActual, "W")) // chequeo si esta en write
                {
                    // archivo en uso, blockeado
                    //  aca tendria que agregar una cola de archivos en espera para leer
                    agregarTablaArchivos(lista_espera_archivos, pcbActual, "R");

                    pthread_mutex_lock(&mutexBlocked);
                    push_estado(blocked, pcbActual);
                    pthread_mutex_unlock(&mutexBlocked);

                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    cambio_de_estado(pcbActual, "BLOCKED");
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_info(kernelLogger, "PID: %d - Bloqueado por:  %s", pcbActual->pid, pcbActual->archivo);
                    log_info(kernelLogger, "pcb blockedado porque el archivo solicitado no esta disponible");
                    //pthread_mutex_unlock(&mutexLoggerKernel);
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);

                    sem_post(&execute_libre);
                }
                else
                {
                    f_open_instruccion(pcbActual->archivo);
                    agregarTablaArchivos(lista_global_archivos, pcbActual, "R");
                    if (!archivoNombreAbiertoLista(lista_punteros_archivos, pcbActual))
                    {
                        agregarTablaArchivos(lista_punteros_archivos, pcbActual, "R");
                    }

                    sem_post(&semExecute);
                }

                pthread_mutex_unlock(&mutex_tabla_archivos);

                break;
            case i_fopen_w:
                pthread_mutex_lock(&mutex_tabla_archivos);
                if (archivoAbiertoLista(lista_global_archivos, pcbActual))
                {
                    // archivo en uso, blockeado
                    //  aca tendria que agregar una cola de archivos en espera para escribir
                    agregarTablaArchivos(lista_espera_archivos, pcbActual, "W"); // se agrega a lista de pcbs esperando para usar el archivo

                    pthread_mutex_lock(&mutexBlocked);
                    push_estado(blocked, pcbActual);
                    pthread_mutex_unlock(&mutexBlocked);

                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    cambio_de_estado(pcbActual, "BLOCKED");
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_info(kernelLogger, "PID: %d - Bloqueado por:  %s", pcbActual->pid, pcbActual->archivo);
                    log_info(kernelLogger, "pcb blockedado porque el archivo solicitado no esta disponible");
                    //pthread_mutex_unlock(&mutexLoggerKernel);
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);

                    sem_post(&execute_libre);
                }
                else
                {
                    f_open_instruccion(pcbActual->archivo);
                    agregarTablaArchivos(lista_global_archivos, pcbActual, "W");
                    if (!archivoNombreAbiertoLista(lista_punteros_archivos, pcbActual))
                    {
                        agregarTablaArchivos(lista_punteros_archivos, pcbActual, "W");
                    }
                    // mando el pcb a exec
                    sem_post(&semExecute);
                }

                pthread_mutex_unlock(&mutex_tabla_archivos);

                break;

            case i_fclose:
                f_close(pcbActual);
                sem_post(&semExecute);

                break;

            case i_fseek:
                pthread_mutex_lock(&mutex_tabla_archivos);
                actualizarFSeek(pcbActual);

                pthread_mutex_unlock(&mutex_tabla_archivos);
                // mando el pcb a exec
                sem_post(&semExecute);
                break;

            case i_fread:
                pthread_mutex_lock(&mutex_tabla_archivos);
                // chequear si esta abierto para leer o no

                if (archivoAbiertoLista(lista_global_archivos, pcbActual))
                {
                    fs_kernel_data *data = malloc(sizeof(*data));
                    data->nombreArchivo = pcbActual->archivo;
                    data->puntero = obtenerPosicionPuntero(pcbActual);
                    data->direcFisica = pcbActual->direccion;
                    data->pid = pcbActual->pid;
                    data->flag = i_read;
                    enviar_fs_kernel(data, kernelConf->socket_filesystem, i_read);

                    // blocked
                    pthread_mutex_lock(&mutexBlocked);
                    push_estado(blocked, pcbActual);
                    pthread_mutex_unlock(&mutexBlocked);

                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    cambio_de_estado(pcbActual, "BLOCKED");
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_info(kernelLogger, "PID: %d - Bloqueado por:  %s", pcbActual->pid, pcbActual->archivo);
                    log_info(kernelLogger, "pcb blockedado porque el archivo esta siendo leido");
                    //pthread_mutex_unlock(&mutexLoggerKernel);
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);
                }
                else
                {
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_error(kernelLogger, "El archivo %s no fue abierto para lectura para el pid %d", pcbActual->archivo, pcbActual->pid);
                    //pthread_mutex_unlock(&mutexLoggerKernel);
                    pthread_mutex_lock(&mutexExit);
                    push_estado(exit_estado, pcbActual);
                    pthread_mutex_unlock(&mutexExit);

                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    exit_pcb(pcbActual, exit_estado, "EXECUTE"); // manda el proceso completo a exit
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);
                }
                sem_post(&execute_libre);
                pthread_mutex_unlock(&mutex_tabla_archivos);
                break;

            case i_fwrite:
                pthread_mutex_lock(&mutex_tabla_archivos);
                // chequear si esta abierto para escribir o no
                if (archivoModoAbiertoLista(lista_global_archivos, pcbActual, "W"))
                {
                    fs_kernel_data *data2 = malloc(sizeof(*data2));
                    data2->nombreArchivo = pcbActual->archivo;
                    data2->puntero = pcbActual->puntero_size;
                    data2->direcFisica = pcbActual->direccion;
                    data2->pid = pcbActual->pid;
                    enviar_fs_kernel(data2, kernelConf->socket_filesystem, i_write);

                    pthread_mutex_lock(&mutexBlocked);
                    push_estado(blocked, pcbActual);
                    pthread_mutex_unlock(&mutexBlocked);

                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    cambio_de_estado(pcbActual, "BLOCKED");
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_info(kernelLogger, "PID: %d - Bloqueado por:  %s", pcbActual->pid, pcbActual->archivo);
                    log_info(kernelLogger, "pcb blockedado porque el archivo esta siendo escrito");
                    //pthread_mutex_unlock(&mutexLoggerKernel);
                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);
                    sem_post(&execute_libre);
                }
                else
                {
                    pthread_mutex_unlock(&mutex_tabla_archivos);
                    //pthread_mutex_lock(&mutexLoggerKernel);
                    log_error(kernelLogger, "El archivo %s no fue abierto para escritura para el pid %d", pcbActual->archivo, pcbActual->pid);
                    //pthread_mutex_unlock(&mutexLoggerKernel);
                    pthread_mutex_lock(&mutexExit);
                    push_estado(exit_estado, pcbActual);
                    pthread_mutex_unlock(&mutexExit);

                    pthread_mutex_lock(&mutexExec);
                    pop_estado(exec);
                    pthread_mutex_unlock(&mutexExec);

                    pthread_mutex_lock(&mutex_quantum);
                    quantumActual++;
                    pthread_mutex_unlock(&mutex_quantum);
                    exit_pcb(pcbActual, exit_estado, "EXECUTE"); // manda el proceso completo a exit
                    sem_post(&execute_libre);
                }
                pthread_mutex_unlock(&mutex_tabla_archivos);
                break;

            case i_ftruncate:

                fs_kernel_data *data2 = malloc(sizeof(fs_kernel_data));
                data2->nombreArchivo = strdup(pcbActual->archivo);
                data2->puntero = -1;
                data2->pid = pcbActual->pid;
                data2->info = -1;
                data2->direcFisica = -1;
                data2->tamanioNuevo = pcbActual->puntero_size;
                enviar_fs_kernel(data2, kernelConf->socket_filesystem, i_truncate);
                // blocked
                pthread_mutex_lock(&mutexBlocked);
                push_estado(blocked, pcbActual);
                pthread_mutex_unlock(&mutexBlocked);

                pthread_mutex_lock(&mutexExec);
                pop_estado(exec);
                pthread_mutex_unlock(&mutexExec);

                cambio_de_estado(pcbActual, "BLOCKED");

                //pthread_mutex_lock(&mutexLoggerKernel);
                log_info(kernelLogger, "PID: %d - Bloqueado por:  %s", pcbActual->pid, pcbActual->archivo);
                log_info(kernelLogger, "pcb blockedado porque el archivo esta siendo truncado");
                //pthread_mutex_unlock(&mutexLoggerKernel);
                pthread_mutex_lock(&mutex_quantum);
                quantumActual++;
                pthread_mutex_unlock(&mutex_quantum);
                free(data2->nombreArchivo);
                free(data2);

                sem_post(&execute_libre);

                break;

            case i_exit:
                pthread_mutex_lock(&mutexExit);
                push_estado(exit_estado, pcbActual);
                pthread_mutex_unlock(&mutexExit);

                pthread_mutex_lock(&mutexExec);
                pop_estado(exec);
                pthread_mutex_unlock(&mutexExec);

                exit_pcb(pcbActual, exit_estado, "EXECUTE"); // funcion que esta en estado.c
                pthread_mutex_lock(&mutex_quantum);
                quantumActual++;
                pthread_mutex_unlock(&mutex_quantum);
                sem_post(&execute_libre);

                break;

            case page_fault:
                //pthread_mutex_lock(&mutexLoggerKernel);
                log_info(kernelLogger, "procesando PF");
                //pthread_mutex_unlock(&mutexLoggerKernel);

                pageFaultKerenel(pcbActual);

                sem_post(&execute_libre);

                break;
            case interrupt_cpu_exito:  

                pthread_mutex_lock(&mutexReady);
                push_estado(ready, pcbActual);
                pthread_mutex_unlock(&mutexReady);
                
                pthread_mutex_lock(&mutexExec);
                pop_estado(exec);
                pthread_mutex_unlock(&mutexExec);

                cambio_de_estado(pcbActual, "READY");
                pthread_mutex_lock(&mutex_quantum);
                quantumActual++;
                pthread_mutex_unlock(&mutex_quantum);
                logger_ready(kernelConf->algoritmo_planificacion);
                sem_post(&execute_libre);
                sem_post(&semReady);
                if(condicionPrioridad){
                    sem_post(&interruptFunco);
                    condicionPrioridad = false;
                }

                break;
            case desalojoPorPrioridad: //prioridad
                pthread_mutex_lock(&mutexExec);
                pop_estado(exec);
                pthread_mutex_unlock(&mutexExec);
                if(!condicionPrioridad){
                    pthread_mutex_lock(&mutexExit);
                    push_estado(exit_estado, pcbActual);
                    pthread_mutex_unlock(&mutexExit);
                }
                else {
                    pthread_mutex_lock(&mutexReady);
                    push_estado(ready, pcbActual);
                    pthread_mutex_unlock(&mutexReady);
                    cambio_de_estado(pcbActual, "READY");
                    logger_ready(kernelConf->algoritmo_planificacion);
                    int valor;
                    sem_getvalue(&semReady, &valor);
                    sem_post(&semReady);
                }
                sem_post(&execute_libre);
                sem_post(&interruptFunco);
                
            break;
            default:
                //pthread_mutex_lock(&mutexLoggerKernel);
                log_error(kernelLogger, "instruccion no valida");
                //pthread_mutex_unlock(&mutexLoggerKernel);
                exit_pcb(pcbActual, exit_estado, "EXECUTE");

                sem_post(&execute_libre);
                break;
            }
        }
    
    }
}

bool archivoAbiertoLista(t_list *lista, pcb *pcbBuscado)
{
    int pidBuscado = pcbBuscado->pid;
    for (int i = 0; i < list_size(lista); i++)
    {
        tabla_de_archivos *tabla = list_get(lista, i);
        if (strcmp(tabla->archivo, pcbBuscado->archivo) == 0 && pidBuscado == pcbBuscado->pid)
        {
            return true;
        }
    }
    return false;
}

bool archivoModoAbiertoLista(t_list *lista, pcb *pcbBuscado, char *modo)
{
    tabla_de_archivos *tabla = malloc(sizeof(*tabla));
    tabla = NULL;
    int pidBuscado = pcbBuscado->pid;
    for (int i = 0; i < list_size(lista); i++)
    {
        tabla = list_get(lista, i);
        if (strcmp(tabla->archivo, pcbBuscado->archivo) == 0 && pidBuscado == pcbBuscado->pid && strcmp(tabla->modoApretura, modo) == 0)
        {
            return true;
        }
    }
    return false;
}

bool enUsoModo(t_list *lista, char *archivo, char *modo)
{
    for (int i = 0; i < list_size(lista); i++)
    {
        tabla_de_archivos *tabla = list_get(lista, i);
        if (strcmp(tabla->archivo, archivo) == 0 && strcmp(tabla->modoApretura, modo) == 0)
        {
            return true;
        }
    }
    return false;
}

bool archivoNombreAbiertoLista(t_list *lista, pcb *pcbBuscado)
{ // lo devuelve y lo saca
    for (int i = 0; i < list_size(lista); i++)
    {
        tabla_de_archivos *tabla = list_get(lista, i);
        if (strcmp(tabla->archivo, pcbBuscado->archivo) == 0)
        {
            return true;
        }
    }
    // free(tabla);
    return false;
}

void eliminarPcbUsando(t_list *lista, pcb *pcbUsuario)
{
    for (int i = 0; i < list_size(lista); i++)
    {
        tabla_de_archivos *tabla = list_get(lista, i);

        if (strcmp(tabla->archivo, pcbUsuario->archivo) == 0 && pcbUsuario->pid == tabla->pid)
        {
            list_remove(lista, i);
            //pthread_mutex_lock(&mutexLoggerKernel);
            log_info(kernelLogger, "se elimino el pcb %d de la lista del archivo %s", pcbUsuario->pid, tabla->archivo);
            //pthread_mutex_unlock(&mutexLoggerKernel);
        }
    }
}

bool archivoEnSistema(char *archivo)
{
    tabla_de_archivos *tabla = malloc(sizeof(*tabla));
    tabla = NULL;
    int size = list_size(lista_global_archivos);
    for (int i = 0; i < size; i++)
    {
        tabla = list_get(lista_global_archivos, i);
        if (strcmp(tabla->archivo, archivo) == 0)
        {
            return true;
        }
    }
    // free(tabla);
    return false;
}

void actualizarFSeek(pcb *pcbActual)
{
    char *archivo = pcbActual->archivo;
    int nuevaPosicion = pcbActual->puntero_size;
    tabla_de_archivos *tabla = malloc(sizeof(*tabla));
    tabla = NULL;
    if (archivoNombreAbiertoLista(lista_global_archivos, pcbActual))
    {
        for (int i = 0; i < list_size(lista_punteros_archivos); i++)
        {
            tabla = list_get(lista_punteros_archivos, i);
            if (strcmp(tabla->archivo, archivo) == 0)
            {
                tabla->posicion_puntero = nuevaPosicion;
            }
        }
    }
    else
    {
        char *archivo = pcbActual->archivo;
        int pidActual = pcbActual->pid;
        //pthread_mutex_lock(&mutexLoggerKernel);
        log_info(kernelLogger, "el archivo %s no se abrio para el pcb %d", archivo, pidActual); // esto me rompe, nose pq
        //pthread_mutex_unlock(&mutexLoggerKernel);
    }
}

int obtenerPosicionPuntero(pcb *Pcbarchivo)
{
    tabla_de_archivos *tabla = malloc(sizeof(*tabla));
    if (archivoNombreAbiertoLista(lista_punteros_archivos, Pcbarchivo))
    {
        for (int i = 0; i < list_size(lista_punteros_archivos); i++)
        {
            tabla = list_get(lista_punteros_archivos, i);
            if (strcmp(tabla->archivo, Pcbarchivo->archivo) == 0)
            {
                return tabla->posicion_puntero;
            }
        }
    }
    //pthread_mutex_lock(&mutexLoggerKernel);
    log_error(kernelLogger, "puntero no encontrado para archivo %d", Pcbarchivo->archivo);
    //pthread_mutex_unlock(&mutexLoggerKernel);
}
char *obtenerModoApretura(pcb *Pcbarchivo)
{
    if (archivoAbiertoLista(lista_global_archivos, Pcbarchivo))
    {
        for (int i = 0; i < list_size(lista_global_archivos); i++)
        {
            tabla_de_archivos *tabla = list_get(lista_global_archivos, i);
            if (strcmp(tabla->archivo, Pcbarchivo->archivo) == 0)
            {
                return tabla->modoApretura;
            }
        }
    }
    char *vacio = "";
    return vacio;
}

tabla_de_archivos *findNextTabla(char *modoApreturaAceptado, char *archivo)
{
    for (int i = 0; i < list_size(lista_espera_archivos); i++)
    {
        tabla_de_archivos *tabla = list_get(lista_espera_archivos, i);
        if (strcmp(tabla->modoApretura, modoApreturaAceptado) != 0 && strcmp(tabla->archivo, archivo) == 0)
        {
            return tabla;
        }
    }
    tabla_de_archivos *tabla2 = malloc(sizeof(tabla_de_archivos));
    tabla2->modoApretura = "Vacio";
    return tabla2;
}

void *f_close(pcb *pcbActual)
{
    pcb* pcbEstadoPrio;
    pthread_mutex_lock(&mutex_tabla_archivos);
    //pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "cerrando archivo");
    //pthread_mutex_unlock(&mutexLoggerKernel);
    char *archivoCerrado = pcbActual->archivo;
    size_t length = strcspn(archivoCerrado, "\n");
    int tipo_archivo = 0;

    // Remove the newline character if found
    if (archivoCerrado[length] == '\n')
    {
        archivoCerrado[length] = '\0';
    }
    char *modoApreturaPcb = obtenerModoApretura(pcbActual); // me guardo como se estaba usando el archivo
    eliminarPcbUsando(lista_global_archivos, pcbActual);    // elimino de la tabla el pcb
    tabla_de_archivos *proximaTabla = findNextTabla(modoApreturaPcb, archivoCerrado);

    if (strcmp("Vacio", proximaTabla->modoApretura) == 0)
    {
        //pthread_mutex_lock(&mutexLoggerKernel);
        log_info(kernelLogger, "no habia pcbs esperando por el archivo");
        //pthread_mutex_unlock(&mutexLoggerKernel);
        free(proximaTabla);
        eliminarPcbUsando(lista_punteros_archivos, pcbActual);
    }
    else if (!enUsoModo(lista_global_archivos, archivoCerrado, "R") && strcmp("W", proximaTabla->modoApretura) == 0)
    {
        //pthread_mutex_lock(&mutexLoggerKernel);
        log_info(kernelLogger, "habia procesos esperando para escribir el archivo cerrado");
        //pthread_mutex_unlock(&mutexLoggerKernel);
        pcb *pcbDesbloqueado = pop_blocked(proximaTabla->pid, blocked);
        agregarTablaArchivos(lista_global_archivos, pcbDesbloqueado, "W");
        eliminarPcbUsando(lista_espera_archivos, pcbDesbloqueado);
        f_open_instruccion(pcbDesbloqueado->archivo);
        pcbDesbloqueado->status = "READY";

        pthread_mutex_lock(&mutexReady);
        push_estado(ready, pcbDesbloqueado);
        pthread_mutex_unlock(&mutexReady);
        cambio_de_estado(pcbDesbloqueado, "BLOCKED");
        
        
            sem_post(&semReady);
        
        
        
    }
    else if (!enUsoModo(lista_global_archivos, archivoCerrado, "W") && strcmp("R", proximaTabla->modoApretura) == 0)
    {
        pcb *pcbDesbloqueado2 = malloc(sizeof(pcb));
        int cantidadDeReady = 0;

        //pthread_mutex_lock(&mutexLoggerKernel);
        log_info(kernelLogger, "habia procesos esperando archivos para leer, chequeando si son el archivo cerrado ");
        //pthread_mutex_unlock(&mutexLoggerKernel);
        pcbDesbloqueado2 = pop_blocked(proximaTabla->pid, blocked);
        agregarTablaArchivos(lista_global_archivos, pcbDesbloqueado2, "R");
        eliminarPcbUsando(lista_espera_archivos, pcbDesbloqueado2);
        f_open_instruccion(pcbDesbloqueado2->archivo);
        pcbDesbloqueado2->status = "READY";

        pthread_mutex_lock(&mutexReady);
        push_estado(ready, pcbDesbloqueado2);
        pthread_mutex_unlock(&mutexReady);

        cambio_de_estado(pcbDesbloqueado2, "BLOCKED");

        cantidadDeReady++;
        for (int i = 0; i < list_size(lista_espera_archivos); i++)
        { // saca todos los q estaban esperando para leer y les da permiso de lectura
            tabla_de_archivos *proximaTabla2 = findNextTabla(modoApreturaPcb, archivoCerrado);
            if (strcmp("Vacio", proximaTabla2->modoApretura) != 0)
            {
                pcbDesbloqueado2 = pop_blocked(proximaTabla2->pid, blocked);
                agregarTablaArchivos(lista_global_archivos, pcbDesbloqueado2, "R");
                eliminarPcbUsando(lista_espera_archivos, pcbDesbloqueado2);
                f_open_instruccion(pcbDesbloqueado2->archivo);
                pcbDesbloqueado2->status = "READY";

                pthread_mutex_lock(&mutexReady);
                push_estado(ready, pcbDesbloqueado2);
                pthread_mutex_unlock(&mutexReady);

                cambio_de_estado(pcbDesbloqueado2, "BLOCKED");
                cantidadDeReady++;
            }
        }
       
        for(int k=0; k <cantidadDeReady;k++){
                sem_post(&semReady);
            }
        
    }

    pthread_mutex_unlock(&mutex_tabla_archivos);
}

/*
void desbloquearIndependientePrioridad(int cantidadDeReady){
            for(int k=0; k <cantidadDeReady;k++){
                sem_post(&semReady);
            }
            //desalojarPrioridad(cantidadDeReady);
            
}*/

void agregarTablaArchivos(t_list *listaObj, pcb *NuevoMiembro, char *modoApretura)
{
    tabla_de_archivos *nuevaTabla = malloc(sizeof(tabla_de_archivos));
    nuevaTabla->pid = NuevoMiembro->pid;
    nuevaTabla->archivo = strdup(NuevoMiembro->archivo);
    nuevaTabla->modoApretura = strdup(modoApretura);
    nuevaTabla->posicion_puntero = 0;
    list_add(listaObj, nuevaTabla);
}
