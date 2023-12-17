#ifndef MEMORIA_DE_INSTRUCCIONES_H_
#define MEMORIA_DE_INSTRUCCIONES_H_

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
#include <utils/flags.h>
#include <dirent.h>
#include "configMemoria.h"


t_instruccion* generar_instruccion_de_a_1(FILE* path);
char* leo_archivo_pseudocodigo(char* archivoIndicado);


#endif
