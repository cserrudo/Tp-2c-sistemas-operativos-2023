
#include "hs.h"

int recibirHS(int socket, handshakes hs, t_log* logger, char* objetivo, char* fuente){
	handshakes handshakeRecibido = stream_recv_header(socket);
	stream_recv_empty_buffer(socket);
			if(handshakeRecibido != hs){
				log_error(logger, "error recibiendo handshake conectando %s con %s", objetivo, fuente);
				return -1;
			}
			log_info(logger, "se recibio hs conectando %s con %s", objetivo, fuente);
			stream_send_empty_buffer(socket, hs_aux);
			return 1;
}

void enviarHS(int socket, handshakes hs, t_log* logger, char* objetivo, char* fuente){
	stream_send_empty_buffer(socket, hs);
	handshakes cpuRta = stream_recv_header(socket);
	stream_recv_empty_buffer(socket);
	if(cpuRta != hs_aux){
		log_error(logger, "error enviando handshake: conectando %s con %s", objetivo, fuente);
		exit(-1);
	}
}