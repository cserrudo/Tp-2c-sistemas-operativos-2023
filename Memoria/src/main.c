#include <stdlib.h>
#include <stdio.h>
#include "configMemoria.h"

#define PATH_CONFIGS "memoria.cfg"

t_log* memoriaLogger;
memoria_config* memoriaConfig;

int main(int argc, char** argv) {
    memoriaLogger = log_create("memoria.log", "Memoria", true, LOG_LEVEL_DEBUG);
    memoriaConfig = memoria_config_crear(argv[1]);
    inicializar_memoria_principal();
    hacerConexiones(memoriaConfig);
    void iniciar_semaphoroPeticiones();
    log_info(memoriaLogger, "conexiones realizadas con exito en memoria");
    liberar_memoria_principal();
    return 0;
}

