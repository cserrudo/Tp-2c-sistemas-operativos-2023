#ifndef FLAGS_H
#define FLAGS_H

typedef enum
{
	hs_aux,
	hs_cpu_dispatch,
	hs_cpu_interrupt,
	hs_memoria,
	hs_filesystem,
	hs_kernel,
}handshakes;

typedef enum
{
	execute_cpu,
	interrupt_cpu_exito,
	interrupt_cpu_fallo,
	desalojar_pcb,
	i_set,
	i_add,
	i_sum,
	i_sub,
	i_jnz,
	i_sleep,
	i_wait,
	i_signal,
	i_mov_in,
	i_mov_out,
	i_fopen_r,
	i_fopen_w,
	i_fclose,
	i_fseek,
	i_fread,
	i_fwrite,
	i_ftruncate,
	i_exit,
	page_fault,
	desalojo_prioridad,
	desalojoPorPrioridad,
}kernel_cpu_dato;

typedef enum{
	obtener_direccion,
	solicitud_marco,
	resultado_mmu,
}i_cpu_memoria;


typedef enum{
	i_open,
	i_create,
	i_truncate,
	i_write,
	i_read,
	creacion_ok,
	truncate_ok,
	lectura_ok,
	escritura_ok,
	finalizacion_ok,
	archivo_inexsitente,
	apertura_ok,
	ejecuta_ok,
}kernel_fs_instruccion;

typedef enum{
	i_iniciar_proceso,
	i_finalizar_proceso,
	finalizacion_ok_fs,
	i_page_fault,
	i_escribir_swap,
	i_direccion_solicitada,
	MARCO_EXITO_FS, 
	ESCRITURA_EXITO_FS,
	}memoria_fs_instruccion;




typedef enum{
	INICIAR_PROCESO_CPU,
	INICIAR_PROCESO_KERNEL,
	FINALIZAR_PROCESO,
	ACCESO_A_ESPACIO_USUARIO,
	PAGE_FAULT, //Para comunicacion de memoria cpu (mmu)
	MARCO_EXITO, //Para comunicacion de memoria cpu (mmu)
	ESCRITURA_EXITO,
	MEMORIA_DONE,
	SOLICITUD_INSTRUCCION,
	ACCESO_A_ESPACIO_USUARIO_LECTURA,
	ACCESO_A_ESPACIO_USUARIO_ESCRITURA,
	ACCESO_A_TABLA,
	SOLUCIONAR_PAGE_FAULT,
	NO_OK
}client_flag;

#endif
