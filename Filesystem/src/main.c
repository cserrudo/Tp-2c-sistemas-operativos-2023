#include <main.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PATH_CONFIGS "filesystem.cfg"

t_log *fsLogger;
fs_config *fsConfig;
void *mapeo_tabla_fat;

void *mapeo_bloques_fat;

int main(int argc, char **argv)
{
    fsLogger = log_create("bin/filesystem.log", "Filesystem", true, LOG_LEVEL_DEBUG);
    fsConfig = fs_config_crear(argv[1]);
    log_info(fsLogger, "Funciono el logger");
    hacerConexiones(fsConfig);

    log_info(fsLogger, "conexiones realizadas con exito en FileSystem");

    verificarTablaFAToCrearla();

    verificarArchivoDeBloquesoCrearlo();

    // crear archivo de bloques

    // crear hilos para atender peticiones de modulo memoria y kernel
    pthread_t atenderKernel;
    pthread_create(&atenderKernel, NULL, (void *)atenderKernelPeticiones, NULL);
    pthread_detach(atenderKernel);

    pthread_t atenderMemoria;
    pthread_create(&atenderMemoria, NULL, (void *)atenderMemoriaPeticiones, NULL);
    pthread_join(atenderMemoria, NULL);

    return 0;
}

void verificarTablaFAToCrearla()
{
    // int archivoViejo = existeArchivo(fsConfig->path_fat);

    int tamanio_fat = (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * sizeof(uint32_t);

    int fd = open(fsConfig->path_fat, O_CREAT | O_RDWR, 0750);
    if (fd == -1)
    {
        log_error(fsLogger, "error al abrir/crear el archivo de la FAT");
    }
    struct stat f_stat;
    fstat(fd, &f_stat);
    if (f_stat.st_size == 0)
    {
        if (ftruncate(fd, tamanio_fat) == -1)
        {
            // error al truncar
            log_error(fsLogger, "error al truncar la FAT");
        }
    }

    mapeo_tabla_fat = mmap(NULL, tamanio_fat, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapeo_tabla_fat == MAP_FAILED)
    {
        log_error(fsLogger, "error al mapear la FAT");
    }

    close(fd);
    //if(!archivoViejo){//inicializamos fat en cero
        uint32_t aux = 0;
         for(int i=0; i<(fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap); i++){
            memcpy(mapeo_tabla_fat+i*sizeof(uint32_t), &aux, sizeof(uint32_t));
         }
    //}
    // se finalizaria con munmap(); para el tema de memory leaks
    /*uint32_t bloque_libre;
    int i=0;
   while (i < (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap)) {
    memcpy(&bloque_libre, mapeo_tabla_fat + (i * sizeof(uint32_t)), sizeof(uint32_t));    
    printf("i: %u, bloque_libre: %u\n", i, bloque_libre);
    i++;
    }*/
}

void verificarArchivoDeBloquesoCrearlo()
{
    int tamanio_archivo_bloques = fsConfig->cant_bloques_total * fsConfig->tam_bloque;

    int fd = open(fsConfig->path_bloques, O_CREAT | O_RDWR, 0750);
    if (fd == -1)
    {
        log_error(fsLogger, "error al abrir/crear el archivo de bloques");
    }
    struct stat f_stat;
    fstat(fd, &f_stat);
    if (f_stat.st_size == 0)
    {
        if (ftruncate(fd, tamanio_archivo_bloques) == -1)
        {
            // error al truncar
            log_error(fsLogger, "error al truncar el archivo de bloques");
        }
    }
    mapeo_swap = mmap(NULL, fsConfig->cant_bloques_swap * fsConfig->tam_bloque, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapeo_swap == MAP_FAILED)
    {
        log_error(fsLogger, "error al mapear el swap");
    }
    mapeo_bloques_fat = mmap(NULL, (fsConfig->cant_bloques_total - fsConfig->cant_bloques_swap) * fsConfig->tam_bloque, PROT_READ | PROT_WRITE, MAP_SHARED, fd, fsConfig->cant_bloques_swap * fsConfig->tam_bloque);
    if (mapeo_bloques_fat == MAP_FAILED)
    {
        log_error(fsLogger, "error al mapear los bloques de fat(archivo bloques)");
    }
    close(fd);
    // se finalizaria con munmap(); para el tema de memory leaks
}

int existeArchivo(char *path_to_file)
{
    FILE *file;
    if (file = fopen(path_to_file, "r"))
    {
        return true;
    }
    return false;
}
