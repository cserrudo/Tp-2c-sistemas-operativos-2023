#include "ciclo_de_instruccion.h"

extern cpu_config *cpuConf;
extern t_log *cpuLogger;

extern pthread_mutex_t mutex_interrupt;

bool sigo_ejecutando = true;
bool hay_interrupcion = false;
bool enviar_interrup = false;
bool hay_prioridad = false;

int AX, BX, CX, DX;

t_instruccion* convertir_data_a_instruccion(memoria_cpu_data *data_cpu) {
	t_instruccion *instruccion = malloc(sizeof(*instruccion));

	instruccion->identificador = data_cpu->programCounter;
	instruccion->param1 = data_cpu->param1;
	instruccion->param2 = data_cpu->param2;
	instruccion->param3 = data_cpu->param3;

	return instruccion;
}

uint32_t buscar_registro_entero(registros_cpu *registros, char *registro) {
	int valor = 0;
	char *token = strtok(registro, "\n");
	if (string_equals_ignore_case(registro, "AX"))
		valor = registros->AX;

	if (string_equals_ignore_case(registro, "BX"))
		valor = registros->BX;

	if (string_equals_ignore_case(registro, "CX"))
		valor = registros->CX;

	if (string_equals_ignore_case(registro, "DX"))
		valor = registros->DX;

	return valor;
}

t_instruccion* fetch(pcb *pcb) {
	memoria_cpu_data *data_cpu = malloc(sizeof(*data_cpu));

	data_cpu->flag = INICIAR_PROCESO_CPU; //esto va?
	int pid = pcb->pid;
	data_cpu->pid = pid;
	int pc = pcb->programCounter;
	data_cpu->programCounter = pc;
	data_cpu->param1 = "";
	data_cpu->param2 = "";
	data_cpu->param3 = "";
	data_cpu->direccion = 0;
	data_cpu->registroValor = 0;
	enviar_memoria_cpu(data_cpu, cpuConf->socket_memoria, INICIAR_PROCESO_CPU);

	memoria_cpu_data *data_cpu2 = recibir_memoria_cpu(cpuConf->socket_memoria);

	t_instruccion *instruccion_recibida = convertir_data_a_instruccion(
			data_cpu2);

	pcb->programCounter += 1;

	free(data_cpu);
	free(data_cpu2);

	return instruccion_recibida;
}
/*
 uint32_t* obtenerRegistro(registros_cpu *registros, char *nombreRegistro)
 {
 if (strcmp(nombreRegistro, "AX") == 0)
 return &registros->AX;
 else if (strcmp(nombreRegistro, "BX") == 0)
 return &registros->BX;
 else if (strcmp(nombreRegistro, "CX") == 0)
 return &registros->CX;
 else if (strcmp(nombreRegistro, "DX") == 0)
 return &registros->DX;
 else
 return 0;
 }*/

void setRegistroValor(registros_cpu *registros, char *nombreRegistro,
		uint32_t valor) {
	if (strcmp(nombreRegistro, "AX") == 0) {
		registros->AX = valor;
	}

	else if (strcmp(nombreRegistro, "BX") == 0) {
		registros->BX = valor;
	}

	else if (strcmp(nombreRegistro, "CX") == 0) {
		registros->CX = valor;
	} else if (strcmp(nombreRegistro, "DX") == 0) {
		registros->DX = valor;
	}

}

void ejecutar_wait(char *recurso, pcb *pcb) {
	char *r = malloc(strlen(recurso) + 1);
	strcpy(r, recurso);
	pcb->recurso = r;
	pcb->resource_requested = r;
	enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, i_wait);
	free(r);
}
void ejecutar_signal(char *recurso, pcb *pcb) {
	char *r = malloc(strlen(recurso) + 1);
	strcpy(r, recurso);
	pcb->recurso = r; // como se libera el recurso entonces queda vacío
	enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, i_signal);
	free(r);
}

char* remove_newline(char *str) {
	char *newline = strchr(str, '\n');
	if (newline != NULL) {
		*newline = '\0';
	}
	return str;
}

void ejecutar_fopen(char *archivo, char *modoApretura, pcb *pcbAux) {
	char *r1 = malloc(strlen(archivo) + 1);
	strcpy(r1, archivo);
	pcbAux->archivo = r1;
	modoApretura = remove_newline(modoApretura);
	if (strcmp(modoApretura, "W") == 0) {
		enviar_cpu_kernel(pcbAux, cpuConf->socket_kernel_dispatch, i_fopen_w);
	} else if (strcmp(modoApretura, "R") == 0) {
		enviar_cpu_kernel(pcbAux, cpuConf->socket_kernel_dispatch, i_fopen_r);
	}

	free(r1);
}

void ejecutar_fclose(char *archivo, pcb *pcbAux) {
	char *r1 = malloc(strlen(archivo) + 1);
	strcpy(r1, archivo);
	pcbAux->archivo = r1;
	pcbAux->file_requested = "";
	enviar_cpu_kernel(pcbAux, cpuConf->socket_kernel_dispatch, i_fclose);

	free(r1);
}

void ejecutar_fseek(char *archivo, char *posicionString, pcb *pcbAux) {
	int posicion = atoi(posicionString);
	char *r1 = malloc(strlen(archivo) + 1);
	strcpy(r1, archivo);
	pcbAux->archivo = r1;
	pcbAux->puntero_size = posicion;
	enviar_cpu_kernel(pcbAux, cpuConf->socket_kernel_dispatch, i_fseek);
	free(r1);
}

void ejecutar_fread(char *archivo, uint32_t dir_logica, pcb *pcbAux) {
	pcbAux->direccion = dir_logica;
	char *r1 = malloc(strlen(archivo) + 1);
	strcpy(r1, archivo);
	pcbAux->archivo = r1;
	enviar_cpu_kernel(pcbAux, cpuConf->socket_kernel_dispatch, i_fread);
	free(r1);
}
void ejecutar_fwrite(char *archivo, uint32_t dir_logica, pcb *pcbAux) {
	pcbAux->direccion = dir_logica;
	char *r1 = malloc(strlen(archivo) + 1);
	strcpy(r1, archivo);
	pcbAux->archivo = r1;
	pcbAux->file_requested = r1;
	enviar_cpu_kernel(pcbAux, cpuConf->socket_kernel_dispatch, i_fwrite);
	free(r1);
}

void ejecutar_ftruncate(char *archivo, char *sizeString, pcb *pcbAux) {
	int size = atoi(sizeString);
	pcbAux->puntero_size = size;
	char *r1 = malloc(strlen(archivo) + 1);
	strcpy(r1, archivo);
	pcbAux->archivo = r1;
	enviar_cpu_kernel(pcbAux, cpuConf->socket_kernel_dispatch, i_ftruncate);
	free(r1);
}

void decode(t_instruccion *instruccion, pcb *pcb) {
	int dir_logica;
	int32_t direccionFisica;
	switch (instruccion->identificador) {
	case SET:

		log_info(cpuLogger, "PID: %d - Ejecutando: SET - %s - %s", pcb->pid,
				instruccion->param1, instruccion->param2);
		setRegistroValor(pcb->registros_cpu, instruccion->param1,
				atoi(instruccion->param2));

		break;
	case SUM:

		log_info(cpuLogger, "PID: %d - Ejecutando: SUM - %s - %s", pcb->pid,
				instruccion->param1, instruccion->param2);
		uint32_t destino_int = buscar_registro_entero(pcb->registros_cpu,
				instruccion->param1);
		uint32_t origen_int = buscar_registro_entero(pcb->registros_cpu,
				instruccion->param2);

		uint32_t suma = destino_int + origen_int;

		setRegistroValor(pcb->registros_cpu, instruccion->param1, suma);

		break;

	case SUB:
		log_info(cpuLogger, "PID: %d - Ejecutando: SUB - %s - %s", pcb->pid,
				instruccion->param1, instruccion->param2);
		uint32_t destino = buscar_registro_entero(pcb->registros_cpu,
				instruccion->param1);
		uint32_t origen = buscar_registro_entero(pcb->registros_cpu,
				instruccion->param2);

		uint32_t resta = destino - origen;

		setRegistroValor(pcb->registros_cpu, instruccion->param1, resta);
		break;
	case JNZ:
		log_info(cpuLogger, "PID: %d - Ejecutando: JNZ - %s - %s", pcb->pid,
				instruccion->param1, instruccion->param2);
		uint32_t valor = buscar_registro_entero(pcb->registros_cpu,
				instruccion->param1);

		if (valor != 0) {
			//log_info(cpuLogger, "se ejecuto JNZ para el registro %s y su instruccion destino es %s", instruccion->param1, instruccion->param2);
			pcb->programCounter = atoi(instruccion->param2);
			//log_info(cpuLogger, "Instruccion JNZ ejecutada...");
		} else {
			//log_info(cpuLogger, "REGISTRO CON VALOR 0");
		}

		break;
	case SLEEP:
		//log_info(cpuLogger, "Instruccion SLEEP ejecutada...");
		log_info(cpuLogger, "PID: %d - Ejecutando: SLEEP - %s ", pcb->pid,
				instruccion->param1);
		pcb->sleep = atoi(instruccion->param1);
		enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, i_sleep);
		break;
	case WAIT:
		//log_info(cpuLogger, "PID: %d - Ejecutando: WAIT - Parametros: %s", pcb->pid, instruccion->param1);
		log_info(cpuLogger, "PID: %d - Ejecutando: WAIT - %s ", pcb->pid,
				instruccion->param1);
		ejecutar_wait(instruccion->param1, pcb);
		break;
	case SIGNAL:
		//log_info(cpuLogger, "PID: %d - Ejecutando: SIGNAL - Parametros: %s", pcb->pid, instruccion->param1);
		log_info(cpuLogger, "PID: %d - Ejecutando: WAIT - %s ", pcb->pid,
				instruccion->param1);
		ejecutar_signal(instruccion->param1, pcb);
		break;
	case MOV_IN:

		log_info(cpuLogger, "PID: %d - Ejecutando: MOV_IN - %s - %s ", pcb->pid,
				instruccion->param1, instruccion->param2);
		dir_logica = atoi(instruccion->param2);
		int dir_fisica = 0;
		int num_pagina = floor(dir_logica / cpuConf->tam_pagina);

		dir_fisica = mmu_execute(dir_logica, pcb->pid);
		if (dir_fisica == -2) {
			//page fault
			pcb->direccion = num_pagina;
			log_error(cpuLogger, "PAGE FAULT: ocurrio en la pagina %d",
					num_pagina);
			enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, page_fault);
			instruccion->identificador = PF;
		} else {
			memoria_cpu_data *data = malloc(sizeof(memoria_cpu_data));
			data->direccion = dir_fisica;
			data->param1 = "";
			data->param2 = "";
			data->param3 = "";
			int pidMovin = pcb->pid;
			data->pid = pidMovin;
			enviar_memoria_cpu(data, cpuConf->socket_memoria,
					ACCESO_A_ESPACIO_USUARIO_LECTURA);

			//recibo la rta

			data = recibir_memoria_cpu(cpuConf->socket_memoria);

			setRegistroValor(pcb->registros_cpu, instruccion->param1,
					data->registroValor);
			free(data);
			//log_info(cpuLogger, "Instruccion MOV_IN ejecutada...");
		}

		break;
	case MOV_OUT:
		log_info(cpuLogger, "PID: %d - Ejecutando: MOV_OUT - %s - %s ",
				pcb->pid, instruccion->param1, instruccion->param2);
		dir_logica = atoi(instruccion->param1);
		int num_paginaOut = floor(dir_logica / cpuConf->tam_pagina);

		dir_fisica = mmu_execute(dir_logica, pcb->pid);
		if (dir_fisica == -2) {
			//page fault
			pcb->direccion = num_paginaOut;
			log_error(cpuLogger, "PAGE FAULT: ocurrio en la pagina %d",
					num_paginaOut);
			enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, page_fault);
			instruccion->identificador = PF;
		} else {
			memoria_cpu_data *dataOUT = malloc(sizeof(memoria_cpu_data));
			dataOUT->direccion = dir_fisica;
			int pidMovin = pcb->pid;
			dataOUT->pid = pidMovin;

			dataOUT->registroValor = buscar_registro_entero(pcb->registros_cpu,
					instruccion->param2);
			dataOUT->param1 = "";
			dataOUT->param2 = "";
			dataOUT->param3 = "";
			dataOUT->programCounter = -1;
			enviar_memoria_cpu(dataOUT, cpuConf->socket_memoria,
					ACCESO_A_ESPACIO_USUARIO_ESCRITURA);

			dataOUT = recibir_memoria_cpu(cpuConf->socket_memoria);
			if (dataOUT->flag != MARCO_EXITO) {
				log_error(cpuLogger,
						"error en la escritura del registro %s el valor %d",
						instruccion->param2, valor);
			}
			free(dataOUT);
		}

		break;
	case F_OPEN:
		log_info(cpuLogger, "PID: %d - Ejecutando: F_OPEN - %s - %s ", pcb->pid,
				instruccion->param1, instruccion->param2);
		ejecutar_fopen(instruccion->param1, instruccion->param2, pcb);
		//log_info(cpuLogger, "Instruccion F_OPEN ejecutada...");
		break;
	case F_CLOSE:
		log_info(cpuLogger, "PID: %d - Ejecutando: F_CLOSE - %s ", pcb->pid,
				instruccion->param1);
		ejecutar_fclose(instruccion->param1, pcb);
		//log_info(cpuLogger, "Instruccion F_CLOSE ejecutada...");
		break;
	case F_SEEK:
		log_info(cpuLogger, "PID: %d - Ejecutando: F_SEEK - %s - %s ", pcb->pid,
				instruccion->param1, instruccion->param2);
		ejecutar_fseek(instruccion->param1, instruccion->param2, pcb);
		//log_info(cpuLogger, "Instruccion F_SEEK ejecutada...");
		break;
	case F_READ:
		log_info(cpuLogger, "PID: %d - Ejecutando: F_READ - %s - %s ", pcb->pid,
				instruccion->param1, instruccion->param2);
		dir_logica = atoi(instruccion->param2);
		direccionFisica = mmu_execute(dir_logica, pcb->pid);
		if (direccionFisica == -2) {
			//page fault
			int num_pagina = floor(dir_logica / cpuConf->tam_pagina);
			pcb->direccion = num_pagina;
			log_error(cpuLogger, "PAGE FAULT: ocurrio en la pagina %d",
					num_pagina);
			enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, page_fault);
			instruccion->identificador = PF;
		} else {
			ejecutar_fread(instruccion->param1, direccionFisica, pcb);
		}
		//log_info(cpuLogger, "Instruccion F_READ ejecutada...");
		break;
	case F_WRITE:
		log_info(cpuLogger, "PID: %d - Ejecutando: F_WRITE - %s - %s ",
				pcb->pid, instruccion->param1, instruccion->param2);
		dir_logica = atoi(instruccion->param2);
		direccionFisica = mmu_execute(dir_logica, pcb->pid);
		if (direccionFisica == -2) {
			//page fault
			int num_pagina = floor(dir_logica / cpuConf->tam_pagina);
			pcb->direccion = num_pagina;
			log_error(cpuLogger, "PAGE FAULT: ocurrio en la pagina %d",
					num_pagina);
			enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, page_fault);
			instruccion->identificador = PF;
		} else {
			ejecutar_fwrite(instruccion->param1, direccionFisica, pcb);
		}
		//log_info(cpuLogger, "Instruccion F_WRITE ejecutada...");
		break;
	case F_TRUNCATE:
		log_info(cpuLogger, "PID: %d - Ejecutando: F_TRUNCATE - %s - %s ",
				pcb->pid, instruccion->param1, instruccion->param2);
		ejecutar_ftruncate(instruccion->param1, instruccion->param2, pcb);
		//log_info(cpuLogger, "Instruccion F_TRUNCATE ejecutada...");
		break;
	case EXIT:
		log_info(cpuLogger, "PID: %d - Ejecutando: EXIT ", pcb->pid);
		//log_info(cpuLogger, "PID: %d - Ejecutando: EXIT", pcb->pid);
		pcb->sleep = 1;
		enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch, i_exit);
		break;
	default:
		break;
	}

}

bool chequear_si_hay_interrupcion() {
	bool interrupcion = false;

	pthread_mutex_lock(&mutex_interrupt);

	if (hay_interrupcion) {
		interrupcion = true;
		enviar_interrup = true;
		hay_interrupcion = false;
	}

	pthread_mutex_unlock(&mutex_interrupt);
	return !interrupcion;
}

bool condicionSigoEjecutando(t_identificador identificador) {
	if (identificador == EXIT || identificador == WAIT
			|| identificador == SIGNAL || identificador == SLEEP
			|| identificador == PF || identificador == F_OPEN
			|| identificador == F_CLOSE || identificador == F_SEEK
			|| identificador == F_READ || identificador == F_WRITE
			|| identificador == F_TRUNCATE) {
		return true;
	}
	return false;
}

void ejecutar_ciclo_de_instruccion(pcb *pcb, int socket_kernel) {

	sigo_ejecutando = 1;
	while (sigo_ejecutando && chequear_si_hay_interrupcion()) {
		//log_info(cpuLogger, "empezo fetch, anterior instruccion: %d ", pcb->programCounter);
		t_instruccion *instruccion = fetch(pcb);
		//log_info(cpuLogger, "termino fetch, proxima instruccion: %d ", pcb->programCounter);

		//log_info(cpuLogger, "empezo decode para instruccion: %d ", pcb->programCounter);
		decode(instruccion, pcb);

		//log_info(cpuLogger, "termino decode para instruccion: %d ", pcb->programCounter);
		if (condicionSigoEjecutando(instruccion->identificador)) {
			sigo_ejecutando = 0;
		}
		free(instruccion);

	}

	if (enviar_interrup && !hay_prioridad) {
		enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch,
				interrupt_cpu_exito); //no se si le tengo que mandar este flag
		enviar_interrup = false;
	} else if (hay_prioridad) {
		enviar_cpu_kernel(pcb, cpuConf->socket_kernel_dispatch,
				desalojoPorPrioridad);
		enviar_interrup = false;
		hay_prioridad = false;
	}
}

