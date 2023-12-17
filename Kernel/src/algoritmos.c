#include "algoritmos.h"

#define cantida_procesos 10

extern kernel_config *kernelConf;
extern t_log *kernelLogger;

extern int quantumActual;
extern int pidActual;

extern pthread_mutex_t mutex_quantum;
extern pthread_mutex_t mutex_pid_global;
extern pthread_mutex_t mutexLoggerKernel;
pcb *prioridades(t_list *ready)
{
    // todo
    int posicionPCB = 0;
    pcb *pcbReturn = list_get(ready, posicionPCB);
    for (int i = 0; i < list_size(ready); i++)
    {
        pcb *pcbAux = list_get(ready, i);
        if (pcbAux->prioridad < pcbReturn->prioridad)
        {
            pcbReturn = pcbAux;
            posicionPCB = i;
        }
    }
    list_remove(ready, posicionPCB);
    return pcbReturn;
}

pcb *procesoConMayorPrioridad(t_list *ready, int prioridadProcesoActual)
{
    pcb *pcbAux;
    pcb* rta;
    for (int i = 0; i < list_size(ready); i++)
    {
         pcbAux= list_get(ready, i);
        if (pcbAux->prioridad < prioridadProcesoActual)
        {
            list_remove(ready, i);
            return pcbAux;
        }
    }
    rta->pid=-1;
    return rta;
}

bool procesoConMayorPrioridadChequeo(t_list *ready, int prioridadProcesoActual)
{
    pcb *pcbAux;
    pcb* rta;
    for (int i = 0; i < list_size(ready); i++)
    {
         pcbAux= list_get(ready, i);
        if (pcbAux->prioridad < prioridadProcesoActual)
        {
            return true;
        }
    }
    return false;
}

void rr_quantum_start(int rafaga)
{
    //pthread_mutex_lock(&mutexLoggerKernel);
    //log_info(kernelLogger, "hilo de rr para pid %d empezado en el turno %d", pidActual, rafaga);
    //pthread_mutex_unlock(&mutexLoggerKernel);
    usleep(kernelConf->quantum*2000);
    //pthread_mutex_lock(&mutexLoggerKernel);
    pthread_mutex_lock(&mutex_quantum);
    //log_info(kernelLogger, "usleep terminado para pid %d , empezo en turno %d y termino en turno %d ", pidActual, rafaga, quantumActual);
    //pthread_mutex_unlock(&mutexLoggerKernel);
    if (rafaga == quantumActual)
    {
        //pthread_mutex_lock(&mutexLoggerKernel);
       // log_info(kernelLogger, "no hubo cambio de turno para pid %d.  Terminando quantum y mandado interrupcion", pidActual);
        //pthread_mutex_unlock(&mutexLoggerKernel);
        terminarQuantum();
    }
    pthread_mutex_unlock(&mutex_quantum);
}

void terminarQuantum()
{
    //pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "enviando interrumpcion para pid %d.", pidActual);
    //pthread_mutex_unlock(&mutexLoggerKernel);
    enviar_interrupt_cpu_kernel(kernelConf->socket_cpu_interrupt, desalojar_pcb);
    pthread_mutex_lock(&mutex_pid_global);
    //pthread_mutex_lock(&mutexLoggerKernel);
    log_info(kernelLogger, "PID: %d Desalojado por fin de Quantum", pidActual);
    //pthread_mutex_unlock(&mutexLoggerKernel);
    pthread_mutex_unlock(&mutex_pid_global);
}

/*
Manejo de variable global con mutex, va aumentado la rafaga por cada
proceso que entre a exec. El hilo que entra a rr, le pas
*/