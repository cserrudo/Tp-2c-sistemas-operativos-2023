#include "peticiones.h"

extern cpu_config* cpuConf;
extern t_log* cpuLogger;

extern bool hay_interrupcion;

extern bool hay_prioridad;

pthread_mutex_t mutex_interrupt;

void atender_dispatch(){
	log_info(cpuLogger, "CPU escuchando puerto dispatch");

	while (1) {
		pcb* contexto = recibir_cpu_kernel(cpuConf->socket_kernel_dispatch);
		switch (contexto->flag) {
			case execute_cpu:
				ejecutar_ciclo_de_instruccion(contexto,cpuConf->socket_kernel_dispatch );
				liberarPcb(contexto);
				break;
			

			default:
				log_error(cpuLogger, "Protocolo invalido, finalizando CPU");
				break;
		}
		//eliminar_paquete(paquete);
	}

}

void atender_interrupt(){
	log_info(cpuLogger, "CPU escuchando puerto interrupt");

	while (1) {
		kernel_cpu_dato cpu_data = recibir_interrupt_cpu_kernel(cpuConf->socket_kernel_interrumpt);
		switch (cpu_data) {
			case desalojar_pcb:
				hay_interrupcion = true;
			break;
			case desalojo_prioridad:
				hay_interrupcion = true;
				hay_prioridad = true;
			break;
			default:
				log_error(cpuLogger, "Protocolo invalido, finalizando CPU");
				break;
			
		}

	}
}
/*
void atenderMemoria(){
	log_info(cpuLogger, "Cpu escuchando memoria");
	memoria_cpu_data* pedido_memoria = malloc(sizeof(memoria_cpu_data));
	while (1) {
		
		pedido_memoria = recibir_memoria_cpu(cpuConf->socket_memoria);
		switch (pedido_memoria->flag) {
			case obtener_direccion:
				int resutlado = mmu_execute(pedido_memoria->direccion, pedido_memoria->pid);
				if(resutlado == -2){
					//page fault
				}
				else if(resutlado = -1){
					//error en rta de marco
				}
				else{
					//exito
					pedido_memoria->direccion = resutlado;
					enviar_memoria_cpu(pedido_memoria, cpuConf->socket_memoria, MARCO_EXITO);
				}
			break;
			default:
				log_error(cpuLogger, "Protocolo invalido, finalizando CPU");
				break;
		}

	}
}*/


uint32_t mmu_execute(int direccionLogica, int pid){

	int sizePagina = cpuConf->tam_pagina;

	//logica para obtener desplazamiento y num de pagina
	int numeroPagina = floor(direccionLogica/sizePagina);
	int desplazamiento = direccionLogica - (numeroPagina*sizePagina);


	//envio solicitud de marco
	memoria_cpu_data* pedido_memoria_marco = malloc(sizeof(*pedido_memoria_marco));
	pedido_memoria_marco->direccion = numeroPagina;
	pedido_memoria_marco->pid = pid;
	pedido_memoria_marco->param1 = "";
	pedido_memoria_marco->param2 = "";
	pedido_memoria_marco->param3 = "";
	enviar_memoria_cpu(pedido_memoria_marco, cpuConf->socket_memoria, ACCESO_A_TABLA);
	
	memoria_cpu_data* data = recibir_memoria_cpu(cpuConf->socket_memoria);

	if(MARCO_EXITO == data->flag){
		//recibo el marco de Memoria
		int marco = data->direccion;

		uint32_t resultado = marco * cpuConf->tam_pagina + desplazamiento;
		return resultado;

	}
	else if(data->flag == PAGE_FAULT){
		return -2;
	}

	else{
		log_error(cpuLogger, "header error, se esperaba PAGE_FAULT o MARCO_EXITO, finalizando programa");
		return -1;
	}
	free(pedido_memoria_marco);
	free(data->param1);
	free(data->param2);
	free(data->param3);
	free(data);
}


