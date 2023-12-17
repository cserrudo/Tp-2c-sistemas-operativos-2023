#include "memoria_de_instrucciones.h"


extern memoria_config* memoriaConfig;
//este path tengo q cambiarlo como en el ejemplo
char* PATH_INSTRUCCIONES = "../Memoria/Directorio/";//path donde se encuentran los archivos de pseudocodigo



char* leo_archivo_pseudocodigo(char* archivoIndicado){//leo del directorio el archivo indicado x el program counter
    DIR *dir = opendir(PATH_INSTRUCCIONES);
    struct dirent *entrada;
    char* archivoConExtension;
    asprintf(&archivoConExtension, "%s.txt", archivoIndicado);
    char* path = NULL;
    asprintf(&path, "%s%s", PATH_INSTRUCCIONES, archivoIndicado);
	free(archivoConExtension);
	closedir(dir);
    return path;
}


t_identificador transformar_identificador(char *identificador) {
	log_info(memoriaLogger, "instruccion: %s", identificador);
	if(string_equals_ignore_case(identificador, "SET")) {
		return SET;
	}
	if(string_equals_ignore_case(identificador, "SUM")) {
		return SUM;
	}
	if(string_equals_ignore_case(identificador, "SUB")) {
		return SUB;
	}
	if(string_equals_ignore_case(identificador, "MOV_IN")) {
		return MOV_IN;
	}
	if(string_equals_ignore_case(identificador, "MOV_OUT")) {
		return MOV_OUT;
	}
	if(string_equals_ignore_case(identificador, "SLEEP")) {
		return SLEEP;
	}
	if(string_equals_ignore_case(identificador, "JNZ")) {
		return JNZ;
	}
	if(string_equals_ignore_case(identificador, "WAIT")) {
		return WAIT;
	}
	if(string_equals_ignore_case(identificador, "SIGNAL")) {
		return SIGNAL;
	}
	if(string_equals_ignore_case(identificador, "SIGNAL")) {
		return SIGNAL;
	}
	if(string_equals_ignore_case(identificador, "F_OPEN")) {
		return F_OPEN;
	}
	if(string_equals_ignore_case(identificador, "F_CLOSE")) {
		return F_CLOSE;
	}
	if(string_equals_ignore_case(identificador, "F_TRUNCATE")) {
		return F_TRUNCATE;
	}
	if(string_equals_ignore_case(identificador, "F_SEEK")) {
		return F_SEEK;
	}
	if(string_equals_ignore_case(identificador, "F_WRITE")) {
		return F_WRITE;
	}
	if(string_equals_ignore_case(identificador, "F_READ")) {
		return F_READ;
	}
	if(string_equals_ignore_case(identificador, "F_CLOSE")) {
		return F_CLOSE;
	}
	if(string_equals_ignore_case(identificador, "EXIT")) {
		return EXIT;
	}
}

t_instruccion* parsear_instruccion(char* linea){
	t_instruccion *instruccion = malloc(sizeof(t_instruccion));
	char** split = string_split(linea, " ");

	instruccion->identificador = transformar_identificador(split[0]);
	
	instruccion->param1 = "";
	instruccion->param2 = "";
	instruccion->param3 = "";
	int i=1;

	while(split[i]!=NULL){
		switch(i){
			case 1:
				instruccion->param1 = strdup(split[i]);
				log_info(memoriaLogger, "parametro 1: %s", instruccion->param1);
				break;
			case 2:
				instruccion->param2 = strdup(split[i]);
				log_info(memoriaLogger, "parametro 2: %s", instruccion->param2);
				break;
			case 3:
				instruccion->param3 = strdup(split[i]);
				log_info(memoriaLogger, "parametro 3: %s", instruccion->param3);
				break;
		}
		i++;
	}
	string_iterate_lines(split, free);
    free(split);
	return instruccion;
}

t_instruccion* generar_instruccion_de_a_1(FILE* archivo){
    char* aux = malloc (1024 * sizeof(char));

    if (archivo == NULL) {
        perror("Error al abrir el archivo de pseudocódigo");
        return 1;
    }
	fgets(aux,1024,archivo);

	t_instruccion* instruccion = parsear_instruccion(aux);

	free(aux);
    return instruccion;
}


/* 
t_instruccion* generar_instruccion_de_a_1(FILE* archivo, int pc) {
    if (archivo == NULL) {
        perror("Error al abrir el archivo de pseudocódigo");
        return NULL; // Return NULL to indicate an error
    }

    char* aux = malloc(1024 * sizeof(char));
    char* auxOtroNombre = malloc(1024 * sizeof(char));

    if (aux == NULL || auxOtroNombre == NULL) {
        // Handle memory allocation error
        free(aux);
        free(auxOtroNombre);
        return NULL;
    }

    // Store the current file position
    long currentPos = ftell(archivo);

    // Set the file position back to the stored position
    fseek(archivo, currentPos, SEEK_SET);

    for (int i = 0; i < pc; i++) {
        if (fgets(aux, 1024, archivo) == NULL) {
            // Handle end of file or read error
            free(aux);
            free(auxOtroNombre);
            return NULL;
        }
    }

    if (fgets(auxOtroNombre, 1024, archivo) == NULL) {
        // Handle end of file or read error
        free(aux);
        free(auxOtroNombre);
        return NULL;
    }

    t_instruccion* instruccion = parsear_instruccion(auxOtroNombre);

    free(aux);
    free(auxOtroNombre);

    return instruccion;
}



 */