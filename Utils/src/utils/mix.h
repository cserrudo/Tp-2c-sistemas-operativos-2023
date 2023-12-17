#ifndef STATIC_HELLO_H_
#define STATIC_HELLO_H_

   
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include <semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <commons/temporal.h>

int config_init(void *moduleConfig, char *pathToConfig, t_log *moduleLogger, void (*config_initializer)(void *moduleConfig, t_config *tempConfig));

#endif
