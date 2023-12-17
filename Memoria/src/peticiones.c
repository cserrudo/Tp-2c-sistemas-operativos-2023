#include "peticiones.h"
#define TAMARRAY 40

extern memoria_config* memoriaConfig;
sem_t semaphores[TAMARRAY];
sem_t pfInfoReady;
extern t_dictionary* diccionario_tablas;
extern t_log* memoriaLogger;
t_list* infoPf;
t_list* procesosMatados;


sem_t semaphores[TAMARRAY];
sem_t pfInfoReady;
t_list* infoPf;
t_list* procesosMatados;
t_list* arrayDeArchivos[TAMARRAY];

int num_marco = 0;
uint32_t info;
int orden = 0;

void iniciar_semaphoroPeticiones(){
	int i;
	for (i = 0; i < TAMARRAY; i++) {
		if (sem_init(&semaphores[i], 0, 0) != 0) {
			perror("Semaphore initialization failed");
			exit(1);
		}
}
}

void procesar_conexion_cpu(int socket_cliente){
	int pid,nroMarco;
	uint32_t dirFisica, valor;
	procesosMatados = list_create();
	while(1){

		//1.kernel manda el archivo a abrir y los bytes q ocupa

		 memoria_cpu_data *pedido_cpu = recibir_memoria_cpu(memoriaConfig->socket_cpu);
		 log_info(memoriaLogger,"se recibio nueva peticion memoria");

		switch(pedido_cpu->flag)
		{
			
			case INICIAR_PROCESO_CPU:
				//primero recibo el archivo indicado devuelvo a pedido instrucciones
				//sem_wait(&creacionFlag); //espera que se termine de armar las estructuras en fs
				sem_wait(&semaphores[pedido_cpu->pid]);
				//log_info(memoriaLogger,"se paso semaforo del elemento archivo memoria");
				//log_info(memoriaLogger,"se abrio el file");
			    
				
				t_list* lista = arrayDeArchivos[pedido_cpu->pid];
				t_instruccion* instruccion = list_get(lista,pedido_cpu->programCounter);

			    pedido_cpu->programCounter = instruccion->identificador;
			    pedido_cpu->param1 = instruccion->param1;
			    pedido_cpu->param2 = instruccion->param2;
			    pedido_cpu->param3 = instruccion->param3;

				//log_info(memoriaLogger, "empezando usleep");
				usleep(memoriaConfig->retardo_respuesta * 1000);
				//log_info(memoriaLogger, "termino usleep");
			    enviar_memoria_cpu(pedido_cpu, memoriaConfig->socket_cpu, pedido_cpu->flag);
			    
				sem_post(&semaphores[pedido_cpu->pid]);

				
				
			break;

			case FINALIZAR_PROCESO:
				FILE* archivo = arrayDeArchivos[pedido_cpu->pid];
				int pid = pedido_cpu->pid;
				list_add(procesosMatados, pid); 
				fclose(archivo);
			break;

			//MOV_OUT desde CPU
			case ACCESO_A_ESPACIO_USUARIO_ESCRITURA:
				//recibo de CPU dir fisica, el valor a escribir y PID
				pid = pedido_cpu->pid;
				dirFisica = pedido_cpu->direccion;
				valor = pedido_cpu->registroValor;
				nroMarco = dirFisica / memoriaConfig->tam_pagina;

				t_marco* marco_escritura = list_get(marcos,nroMarco);

				//if(!marco_escritura->libre){
					t_pagina* pagEscritura = buscar_pag_tabla(marco_escritura->pid_pag, marco_escritura->pid_proceso);

					escribir_espacio_usuario(dirFisica,valor);
					pagEscritura->ultimo_acceso = time(NULL);
					pagEscritura->bit_modificado = 1;
					enviar_memoria_cpu(pedido_cpu, memoriaConfig->socket_cpu, MARCO_EXITO);
					log_info(memoriaLogger,"PID: %d - Accion: <ESCRIBIR> - Direccion fisica: %d",pid,dirFisica);
				//}else{
				//	enviar_memoria_cpu(pedido_cpu, memoriaConfig->socket_cpu, NO_OK);
				//	log_error(memoriaConfig, "no se encontro la pagina");
				//}
				//enviar OK

				

				
			break;
			//MOV IN desde cpu
			case ACCESO_A_ESPACIO_USUARIO_LECTURA:
				//recibo de CPU dir fisica

				pid = pedido_cpu->pid;
				dirFisica = pedido_cpu->direccion;

				nroMarco = dirFisica / memoriaConfig->tam_pagina;

				t_marco* marco_lectura= list_get(marcos,nroMarco);


				//if(!marco_lectura->libre){
					t_pagina* pagLectura = buscar_pag_tabla(marco_lectura->pid_pag, marco_lectura->pid_proceso);

					valor = leer_espacio_usuario(dirFisica);
					pagLectura->ultimo_acceso = time(NULL);

					pedido_cpu->registroValor = valor;
					enviar_memoria_cpu(pedido_cpu, memoriaConfig->socket_cpu, MARCO_EXITO);

					log_info(memoriaLogger,"PID: %d - Accion: <LEER> - Direccion fisica: %d",pid,dirFisica);


				//}else{
				//	enviar_memoria_cpu(pedido_cpu, memoriaConfig->socket_cpu, NO_OK);
					//log_error(memoriaConfig, "no se encontro la pagina");
				//}


			break;
			case ACCESO_A_TABLA:
				//recibo el numero de pagina del proceso buscado de cpu
				int paginaBuscada = pedido_cpu->direccion; //aca le llega el numero de pagina nada mas, hay que buscar que pagina es
				//buscar pagina
				//t_pagina* pagTabla;
				pid = pedido_cpu->pid;
				memoria_cpu_data *data = malloc(sizeof(*data));
				int nro_marco = buscar_pag_memoria(paginaBuscada,pid);
				data->param1 = "";
				data->param2 = "";
				data->param3 = "";
				data->pid=pid;
				data->programCounter=-1;
				data->registroValor=1;
				if(nro_marco>=0){
					//devuelvo el numero de marco a cpu
					data->direccion = nro_marco;
					enviar_memoria_cpu(data, memoriaConfig->socket_cpu,MARCO_EXITO);
				
				}else{
					//devuelvo a  CPU page fault
					data->direccion = -1;
					enviar_memoria_cpu(data, memoriaConfig->socket_cpu,PAGE_FAULT);
				}
			break;




		}
	free(pedido_cpu);
	}
}



void procesar_conexion_kernel(int socket_cliente){
	
	int cant_entradas= 0;
	uint32_t valor = 0;
	int pid = 0;
	t_list* lista_vacia = list_create();
	char* charVacio = "";
	while(1){

		//1.kernel manda el archivo a abrir y los bytes q ocupa
		kernel_memoria_data* data_kernel  =  recibir_kernel_memoria(memoriaConfig->socket_kernel);
		log_info(memoriaLogger, "recibi de kernel en memoria");
		switch(data_kernel->flag)
		{
			case INICIAR_PROCESO_KERNEL: //preparo estructuras necesarias + recibo de kernel
				log_info(memoriaLogger, "se recibio un iniciar procesos");
				FILE* archivo;
				t_list* listaNueva = list_create();
				char* nombre = data_kernel->nombre;
				char* path = leo_archivo_pseudocodigo(nombre);
				//log_info(memoriaLogger, "se ejecuto leo_archivo_pseudocodigo");
				archivo = fopen(path, "r");
				t_instruccion* instruccion;
				instruccion = generar_instruccion_de_a_1(archivo);
				while(instruccion->identificador!= EXIT){
					list_add(listaNueva, instruccion); 
					instruccion = generar_instruccion_de_a_1(archivo);
				}
				list_add(listaNueva, instruccion); 
				
				arrayDeArchivos[data_kernel->pid] = listaNueva;
			    sem_post(&semaphores[data_kernel->pid]);
				//log_info(memoriaLogger, "se parseo las instrucciones");
				fclose(archivo);

				
				cant_entradas = data_kernel->size / memoriaConfig->tam_pagina;
				int pid1 = data_kernel->pid;

				 fs_memoria_data* soliMemoria = malloc(sizeof(*soliMemoria));
				 soliMemoria->pid = pid1;
				 soliMemoria->cantEntradas = cant_entradas;
				 soliMemoria->bloques_asignados = list_create();
				 soliMemoria->direcFisica = 0; 
				soliMemoria->numero_de_bloque = 0; // Initialize to a default value (replace with the appropriate default value)
				soliMemoria->info = 0; 
				 
				

				enviar_fs_memoria(soliMemoria, memoriaConfig->socket_fs, i_iniciar_proceso);
				
				//paginacion, pido a FS bloques
				//lo hace abajo esto ahora

				//osea espera a recibir rta del fs (abajo) y ahi sigue ejecutando

				  
			break;
			case FINALIZAR_PROCESO:
				cant_entradas = data_kernel->size / memoriaConfig->tam_pagina;
				log_info(memoriaLogger, "finalizando proceso");

				finalizar_proceso(data_kernel->pid);

				log_info(memoriaLogger, "PID: %d - Tamaño (cant paginas): %d", data_kernel->pid, cant_entradas);

			break;
			
			case SOLUCIONAR_PAGE_FAULT: 
				//kernel me manda nro de pag y de proceso para guardarlo en la memoria
				//le tengo q pedir a FS el nro de bloque asociado a ese nro de pag
				int pag = data_kernel->pagina;
				pid = data_kernel->pid;

				if(!hay_marcos_libres()){ 
					int marco_victima_index = obtener_marco_victima();
					t_marco* marco_victima = reemplazar_marco_victima(marco_victima_index);

					log_info(memoriaLogger,"REEMPLAZO - Marco: %d - Page Out: (PID %d ) - (NRO_PAG %d) - Page In: (PID %d ) - (NRO_PAG %d)",
						marco_victima->nro_marco, marco_victima->pid_proceso, marco_victima->pid_pag,pag,pid);
		
					marco_victima->libre = 1;
					marco_victima->pid_pag = -1;
					marco_victima->pid_proceso = -1;
					marco_victima->orden = -1;
				}

				t_marco* marco =  buscar_marco_libre();
				//chequear si el marco esta libre

				for(int i = 0; i < list_size(procesosMatados);i++){
					if( list_get(procesosMatados,i) == pid ){
						log_info(memoriaConfig,"el proceso %d que estaba siendo ejecutada, a sido mandado a exit",pid);
						break;
					}
				}
				

				if(marco != NULL){ 
				marco->pid_pag = pag;		
				marco->pid_proceso = pid;
				marco->libre = false;
				
				pthread_mutex_lock(&mutex_orden);
				marco->orden = orden + 1;
				pthread_mutex_unlock(&mutex_orden);

				t_pagina* pagina = buscar_pag_tabla(pag,pid);
								
				pagina->bit_modificado = 0;
				pagina->bit_de_presencia = 1;
				pagina->numero_marco = marco->nro_marco;

				pagina->bit_modificado = 0;
				pagina->bit_de_presencia = 1;
				pagina->numero_marco = marco->nro_marco;

				fs_memoria_data* soliFS = malloc(sizeof(*soliFS));

				soliFS->numero_de_bloque = pagina->nro_bloque;
				soliFS->bloques_asignados = lista_vacia;
				soliFS->pid = pid;
				soliFS->cantEntradas = 0;
				soliFS->direcFisica = 0;
				soliFS->info = 0;
				//soliFS->numero_de_bloque = marco-> aca tenemos que marcar q bloque es para fs
				enviar_fs_memoria(soliFS, memoriaConfig->socket_fs, i_page_fault);
				//log_info(memoriaLogger, "esperando respuesta de fs para page fault");
				//TODO swappeo
				sem_wait(&pfInfoReady);
				valor = info;
				
				log_info(memoriaLogger, "recibiendo valor en posicion de fs %d", valor);
				escribir_en_mem(marco,valor);
				//log_info(memoriaLogger, "se escribio en el marco el valor obtenido");
				//log_info(memoriaLogger, "se acualizo la tabla de marcos");
			
				}
				log_info(memoriaLogger, "enviando a kernel que se soluciono el page fault");

				
				kernel_memoria_data* rta_pf = malloc(sizeof(kernel_memoria_data));
				rta_pf->pagina = 0; //aca le paso la pagina nueva? creo que esta de mas, no le sirve el dato a kernel me pa -mati
				
				rta_pf->nombre = charVacio;
				rta_pf->pid = pid;
				rta_pf->size = 0;
				enviar_kernel_memoria(rta_pf,memoriaConfig->socket_kernel, SOLUCIONAR_PAGE_FAULT);

				break;
		}
		free(data_kernel);
	}
}

void desasignarBloques(t_list *bloquesALiberar,t_list *tabla){
	for(int i=0;i<list_size(tabla);i++){
		t_pagina* pagina = list_get(tabla,i);
		list_add(bloquesALiberar,pagina->nro_bloque); //nro de bloque es un int
	}
}


void finalizar_proceso(int pid) {
	t_list *bloquesALiberar = list_create(); //aca asignar lo q haya q liberar
	log_info(memoriaLogger, "buscando bloques para desasignar");
	char *clave = string_from_format("%d", pid);
	t_list *tabla = dictionary_get(diccionario_tablas, clave);
	log_info(memoriaLogger, "se encontro tabla de bloques asignados");
	desasignarBloques(bloquesALiberar,tabla);

	dictionary_remove(diccionario_tablas, clave);
	list_destroy(tabla);
	free(clave);

	for (int i = 0; i < list_size(marcos); i++) {
		t_marco *marco = list_get(marcos, i);

		if (marco->pid_proceso == pid) {
			marco->libre = true;
			marco->pid_pag = -1;
			marco->pid_proceso = -1;
			marco->orden = -1;
		}
	
	}
	


	//notificar FS que libere bloques SWAP
	//aca necesito que me consigas los bloques a liberar

	fs_memoria_data *data = malloc(sizeof(*data));
	data->bloques_asignados = bloquesALiberar;
	data->cantEntradas = 0;
	data->direcFisica = 0;
	data->info=0;
	data->numero_de_bloque=0;
	data->pid=0;	
	log_info(memoriaLogger, "enviando a fs el fin del proceso");

	enviar_fs_memoria(data, memoriaConfig->socket_fs, i_finalizar_proceso);

} 




void procesar_conexion_fs(int socket_cliente){
	int cant_entradas=0;
	infoPf = list_create();
	sem_init(&pfInfoReady, 0, 0);
	while(1){
		 fs_memoria_data *soliMemoria = recibir_fs_memoria(memoriaConfig->socket_fs);
		 log_info(memoriaLogger,"se recibio nueva peticion memoria desde FileSystem");
		 t_buffer* bufferSoliFs = buffer_create();
		 int pid,nroMarco;
		 uint32_t dirFisica, valor;
		 void* info;
		switch(soliMemoria->flag)
		{
			//MOV_IN O MOV_OUT desde CPU
			case ACCESO_A_ESPACIO_USUARIO_ESCRITURA:
				pid = soliMemoria->pid;
				dirFisica = soliMemoria->direcFisica;
				valor = soliMemoria->info;
				soliMemoria->bloques_asignados = list_create();
				
				nroMarco = dirFisica / memoriaConfig->tam_pagina;

				t_marco* marco_escritura_fs = list_get(marcos,nroMarco);
				//if(!marco_escritura_fs->libre){
					t_pagina* pagEscrituraFS = buscar_pag_tabla(marco_escritura_fs->pid_pag, marco_escritura_fs->pid_proceso);

				escribir_espacio_usuario(dirFisica,valor);
				pagEscrituraFS->ultimo_acceso = time(NULL);
				pagEscrituraFS->bit_modificado = 1;
				enviar_fs_memoria(soliMemoria, memoriaConfig->socket_fs, MARCO_EXITO_FS);
				log_info(memoriaLogger,"PID: %d - Accion: <ESCRIBIR> - Direccion fisica: %d",pid,dirFisica);

			break;
			case ACCESO_A_ESPACIO_USUARIO_LECTURA:
				//recibo de CPU dir fisica
		
				pid = soliMemoria->pid;
				dirFisica = soliMemoria->direcFisica;
				nroMarco = dirFisica / memoriaConfig->tam_pagina;
				t_marco* marco_lectura_fs = list_get(marcos,nroMarco);


				//if(!marco_lectura_fs->libre){
//					t_pagina*  pagLecturaFS = malloc(sizeof(t_pagina))
//					pagLecturaFS = buscar_pag_tabla(marco_lectura_fs->pid_pag, marco_lectura_fs->pid_proceso);
//					pagLecturaFS->ultimo_acceso = time(NULL);

					valor = leer_espacio_usuario(dirFisica);
					soliMemoria->info = valor;
					soliMemoria->bloques_asignados = list_create();
					enviar_fs_memoria(soliMemoria, memoriaConfig->socket_fs, ESCRITURA_EXITO_FS);

					log_info(memoriaLogger,"PID: %d - Accion: <LEER> - Direccion fisica: %d",pid,dirFisica);


				//}else{
				//	enviar_fs_memoria(soliMemoria, memoriaConfig->socket_fs, NO_OK);
				//	log_info(memoriaConfig,"no encontro la pagina");
				//}

			break;
			case i_iniciar_proceso:

				cant_entradas = soliMemoria->cantEntradas;
				pid = soliMemoria->pid;
				cant_entradas = soliMemoria->cantEntradas;
				pid = soliMemoria->pid;
				t_list* bloques = soliMemoria->bloques_asignados;
				//recibe los bloques
				
				

				//int *bloques = (int *)malloc(cant_entradas * sizeof(int)); //array de ints

			    crear_tabla_de_paginacion(cant_entradas, pid,bloques);

				log_info(memoriaLogger, "PID: %d - Tamaño (cant paginas): %d",pid, cant_entradas);

				sem_post(&semaphores[pid]);

			break;
			case i_page_fault:
				info = soliMemoria->info;
				sem_post(&pfInfoReady);
			break;
			case finalizacion_ok_fs:
			
	 		log_info(memoriaLogger,"se libero bloques en FS con exito");
			break;
		}
	}
	}
