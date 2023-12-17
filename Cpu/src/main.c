#include <stdlib.h>
#include <stdio.h>
#include <commons/config.h>
#include<commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include<netdb.h>
#include<commons/log.h>
#include "configCpu.h"
#define PATH_CONFIGS "cpu.cfg"

t_log* cpuLogger;
cpu_config* cpuConf;

int main(int argc, char* argv[]) {
    cpuLogger = log_create("cpu.log", "Cpu", true, LOG_LEVEL_DEBUG);
    cpuConf = cpu_config_crear(PATH_CONFIGS);

   hacerConexiones(cpuConf);

   log_info(cpuLogger, "conexiones realizadas con exito en cpu");


   for(;;){

   }

    return 0;
}
