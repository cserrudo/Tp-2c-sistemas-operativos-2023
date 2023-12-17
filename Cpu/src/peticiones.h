#ifndef PETICIONES_H_
#define PETICIONES_H_

#include <commons/config.h>
#include<commons/collections/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include "configCpu.h"
#include <buffer/send.h>
#include "buffer/buffer.h"
#include <structs/structs.h>
#include <utils/flags.h>
#include "ciclo_de_instruccion.h"
#include <math.h>


void cpu_config_iniciar(void* moduleConfig, t_config* config);
void atender_interrupt();
void atender_dispatch();
uint32_t mmu_execute(int direccionLogica, int pid);
#endif
