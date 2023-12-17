#include <stdlib.h>
#include <stdio.h>
#include <commons/string.h>
#include "planificador.h"
#include <handshakes/hs.h>
#include "configKernel.h"
#include <utils/flags.h>
#include "consola.h"
#define PATH_CONFIGS "kernel.cfg"

t_log *kernelLogger;
kernel_config *kernelConf;

int main(int argc, char **argv)
{
   pthread_mutex_init(&mutexLoggerKernel, NULL);
  pthread_mutex_lock(&mutexLoggerKernel);
  kernelLogger = log_create("bin/kernel.log", "Kernel", true, LOG_LEVEL_DEBUG);
  pthread_mutex_unlock(&mutexLoggerKernel);
  kernelConf = kernel_config_crear(argv[1]);

  hacerConexiones(kernelConf);

  pthread_mutex_lock(&mutexLoggerKernel);
  log_info(kernelLogger, "conexiones realizadas con exito en kernel");
  pthread_mutex_unlock(&mutexLoggerKernel);
  // aca tendria que recibir cosas de los distintos modulos
  inicio_array_recursos();
  iniciar_planificacion();
  
  // que avise si no hay procesos para planificar

  pthread_t manejo_consola;
  pthread_create(&manejo_consola, NULL, (void *)operarConsola, NULL);
  pthread_join(manejo_consola, NULL);
  
  return 0;
}
