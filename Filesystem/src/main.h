#ifndef MAIN_FS_H_
#define MAIN_FS_H_
#include <commons/log.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <utils/flags.h>
#include <stdlib.h>
#include <stdio.h>
#include <configFS.h>
#include <peticionesKernel.h>
#include <peticionesMemoria.h>
void *mapeo_swap;
void verificarTablaFAToCrearla();

void verificarArchivoDeBloquesoCrearlo();

int existeArchivo(char *path_to_file);

#endif
