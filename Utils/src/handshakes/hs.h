#ifndef HS_H_
#define HS_H_
#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <semaphore.h>
#include<string.h>
#include<sys/socket.h>
#include "../utils/flags.h"
#include "../server/server.h"
#include "../buffer/buffer.h"
#include <server/server.h>

int recibirHS(int socket, handshakes hs, t_log* logger, char* objetivo, char* fuente);
void enviarHS(int socket, handshakes hs, t_log* logger, char* objetivo, char* fuente);

#endif
